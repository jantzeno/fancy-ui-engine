#include "fancy_ui/shell/application.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace fancy_ui::shell {

namespace {

void DrawRegion(const RegionSpec &region, const ImVec2 size) {
  if (!region.visible || !region.draw) {
    return;
  }
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const std::string id = detail::Owned(region.id);
  if (region.zero_padding) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  }
  if (region.menu_bar) {
    ImGui::PushFont(ImGui::GetFont(), metrics.menu.font_size);
    const float vertical_padding = std::max(
        0.0f,
        (metrics.shell.application_bar_height - ImGui::GetFontSize()) * 0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(metrics.spacing.space04, vertical_padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(metrics.spacing.space04, 0.0f));
  }
  const ImGuiWindowFlags flags =
      region.menu_bar ? ImGuiWindowFlags_MenuBar : ImGuiWindowFlags_None;
  if (ImGui::BeginChild(id.c_str(), size, ImGuiChildFlags_None, flags)) {
    region.draw();
  }
  ImGui::EndChild();
  if (region.menu_bar) {
    ImGui::PopStyleVar(2);
    ImGui::PopFont();
  }
  if (region.zero_padding) {
    ImGui::PopStyleVar();
  }
}

bool IsDrawable(const RegionSpec &region) {
  return region.visible && static_cast<bool>(region.draw);
}

float PhysicalPixels(const float logical_pixels) {
  return std::round(logical_pixels * CurrentUiScale());
}

float LogicalPixels(const float physical_pixels) {
  return physical_pixels / CurrentUiScale();
}

float ClampPanelWidth(const float requested, const float contract_minimum,
                      const float contract_maximum,
                      const float available_maximum) {
  const float maximum =
      std::max(0.0f, std::min(contract_maximum, available_maximum));
  if (maximum < contract_minimum) {
    return maximum;
  }
  return std::clamp(requested, contract_minimum, maximum);
}

float DrawVerticalSplitter(const char *id, const float height,
                           const LayoutMetrics &metrics) {
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  ImGui::InvisibleButton(id, ImVec2(metrics.shell.splitter_width, height),
                         ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const bool keyboard_focused =
      interaction.focused && ImGui::GetIO().NavVisible;
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const SemanticPalette &palette = CurrentPalette();
  const ColorRgba color =
      interaction.active || interaction.hovered || keyboard_focused
          ? palette.focus
          : palette.border;
  const float line_width =
      interaction.active || interaction.hovered || keyboard_focused
          ? metrics.geometry.focus_ring
          : metrics.geometry.border;
  const float center_x = std::floor((minimum.x + maximum.x) * 0.5f);
  ImGui::GetWindowDrawList()->AddLine(
      ImVec2(center_x, minimum.y), ImVec2(center_x, maximum.y),
      ImGui::GetColorU32(
          ImVec4(color.red, color.green, color.blue, color.alpha)),
      line_width);
  detail::DrawFocusRing(interaction);
  return interaction.active ? ImGui::GetIO().MouseDelta.x : 0.0f;
}

} // namespace

ApplicationShellResult Application(const ApplicationShellSpec &spec,
                                   const ApplicationShellState &state) {
  const LayoutMetrics logical = LogicalLayoutMetrics();
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ApplicationShellResult result{.state = state};
  result.state.operation_tray_height = std::clamp(
      state.operation_tray_height, logical.shell.operation_tray_minimum_height,
      logical.shell.operation_tray_maximum_height);

  DrawRegion(spec.application_bar,
             ImVec2(0.0f, metrics.shell.application_bar_height));
  DrawRegion(spec.context_toolbar,
             ImVec2(0.0f, metrics.shell.context_toolbar_height));

  float reserved_height = metrics.shell.status_bar_height;
  if (IsDrawable(spec.operation_strip)) {
    reserved_height += metrics.shell.operation_strip_height;
  }
  const bool tray_visible =
      state.operation_tray_visible && IsDrawable(spec.operation_tray);
  const float tray_height =
      tray_visible ? PhysicalPixels(result.state.operation_tray_height) : 0.0f;

  const float main_height =
      std::max(0.0f, ImGui::GetContentRegionAvail().y - reserved_height);
  const float main_width = ImGui::GetContentRegionAvail().x;
  const bool rail_visible = IsDrawable(spec.activity_rail);
  const bool explorer_visible =
      state.explorer_visible && IsDrawable(spec.explorer);
  const bool inspector_visible =
      state.inspector_visible && IsDrawable(spec.inspector);

  const float rail_width =
      rail_visible ? metrics.shell.activity_rail_width : 0.0f;
  const float explorer_splitter =
      explorer_visible ? metrics.shell.splitter_width : 0.0f;
  const float inspector_reserve =
      inspector_visible
          ? metrics.shell.splitter_width + metrics.shell.inspector_minimum_width
          : 0.0f;
  float explorer_width =
      explorer_visible ? PhysicalPixels(state.explorer_width) : 0.0f;
  if (explorer_visible) {
    const float explorer_available = main_width - rail_width -
                                     explorer_splitter - inspector_reserve -
                                     metrics.shell.workspace_minimum_width;
    explorer_width = ClampPanelWidth(
        explorer_width, metrics.shell.explorer_minimum_width,
        metrics.shell.explorer_maximum_width, explorer_available);
  }

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginChild("##fancy-ui-main-regions", ImVec2(0.0f, main_height),
                        ImGuiChildFlags_None,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    if (rail_visible) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      DrawRegion(spec.activity_rail, ImVec2(rail_width, main_height));
      ImGui::PopStyleVar();
      ImGui::SameLine(0.0f, 0.0f);
    }

    if (explorer_visible) {
      DrawRegion(spec.explorer, ImVec2(explorer_width, main_height));
      ImGui::SameLine(0.0f, 0.0f);
      const float delta =
          DrawVerticalSplitter("##explorer-splitter", main_height, metrics);
      if (delta != 0.0f) {
        const float available = main_width - rail_width - explorer_splitter -
                                inspector_reserve -
                                metrics.shell.workspace_minimum_width;
        explorer_width = ClampPanelWidth(
            explorer_width + delta, metrics.shell.explorer_minimum_width,
            metrics.shell.explorer_maximum_width, available);
      }
      ImGui::SameLine(0.0f, 0.0f);
    }

    const float right_width = std::max(
        0.0f, main_width - rail_width - explorer_width - explorer_splitter);
    if (ImGui::BeginChild("##fancy-ui-right-stack",
                          ImVec2(right_width, main_height),
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse)) {
      const float top_height = std::max(0.0f, main_height - tray_height);
      float inspector_width =
          inspector_visible ? PhysicalPixels(state.inspector_width) : 0.0f;
      const float inspector_splitter =
          inspector_visible ? metrics.shell.splitter_width : 0.0f;
      if (inspector_visible) {
        inspector_width = ClampPanelWidth(
            inspector_width, metrics.shell.inspector_minimum_width,
            metrics.shell.inspector_maximum_width,
            right_width - inspector_splitter -
                metrics.shell.workspace_minimum_width);
      }
      const float workspace_width =
          std::max(0.0f, right_width - inspector_splitter - inspector_width);

      DrawRegion(spec.workspace, ImVec2(workspace_width, top_height));
      if (inspector_visible) {
        ImGui::SameLine(0.0f, 0.0f);
        const float delta =
            DrawVerticalSplitter("##inspector-splitter", top_height, metrics);
        if (delta != 0.0f) {
          inspector_width = ClampPanelWidth(
              inspector_width - delta, metrics.shell.inspector_minimum_width,
              metrics.shell.inspector_maximum_width,
              right_width - inspector_splitter -
                  metrics.shell.workspace_minimum_width);
        }
        ImGui::SameLine(0.0f, 0.0f);
        DrawRegion(spec.inspector, ImVec2(inspector_width, top_height));
      }

      if (tray_visible) {
        ImGui::SetCursorPosY(top_height);
        DrawRegion(spec.operation_tray, ImVec2(0.0f, tray_height));
      }

      if (explorer_visible) {
        result.state.explorer_width = LogicalPixels(explorer_width);
      }
      if (inspector_visible) {
        result.state.inspector_width = LogicalPixels(inspector_width);
      }
    }
    ImGui::EndChild();
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();

  DrawRegion(spec.operation_strip,
             ImVec2(0.0f, metrics.shell.operation_strip_height));
  DrawRegion(spec.status_bar, ImVec2(0.0f, metrics.shell.status_bar_height));

  result.layout_changed =
      result.state.explorer_width != state.explorer_width ||
      result.state.inspector_width != state.inspector_width ||
      result.state.operation_tray_height != state.operation_tray_height;
  return result;
}

} // namespace fancy_ui::shell
