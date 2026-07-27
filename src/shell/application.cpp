#include "fancy_ui/shell/application.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace fancy_ui::shell {

namespace {

constexpr float kApplicationBarHeight = 40.0f;
constexpr float kContextToolbarHeight = 40.0f;
constexpr float kActivityRailWidth = 48.0f;
constexpr float kOperationStripHeight = 32.0f;
constexpr float kStatusBarHeight = 24.0f;

void DrawRegion(const RegionSpec &region, const ImVec2 size) {
  if (!region.visible || !region.draw) {
    return;
  }
  const std::string id = detail::Owned(region.id);
  if (ImGui::BeginChild(id.c_str(), size, ImGuiChildFlags_None)) {
    region.draw();
  }
  ImGui::EndChild();
}

int MainColumnCount(const ApplicationShellSpec &spec,
                    const ApplicationShellState &state) {
  int count = 1;
  count += spec.activity_rail.visible && spec.activity_rail.draw ? 1 : 0;
  count += state.explorer_visible && spec.explorer.visible && spec.explorer.draw
               ? 1
               : 0;
  count +=
      state.inspector_visible && spec.inspector.visible && spec.inspector.draw
          ? 1
          : 0;
  return count;
}

} // namespace

ApplicationShellResult Application(const ApplicationShellSpec &spec,
                                   const ApplicationShellState &state) {
  ApplicationShellResult result{.state = state};
  result.state.operation_tray_height =
      std::clamp(state.operation_tray_height, 160.0f, 240.0f);
  DrawRegion(spec.application_bar, ImVec2(0.0f, kApplicationBarHeight));
  DrawRegion(spec.context_toolbar, ImVec2(0.0f, kContextToolbarHeight));

  float reserved_height = kStatusBarHeight;
  if (spec.operation_strip.visible && spec.operation_strip.draw) {
    reserved_height += kOperationStripHeight;
  }
  if (state.operation_tray_visible && spec.operation_tray.visible &&
      spec.operation_tray.draw) {
    reserved_height += result.state.operation_tray_height;
  }

  const float main_height =
      std::max(0.0f, ImGui::GetContentRegionAvail().y - reserved_height);
  const int column_count = MainColumnCount(spec, state);
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginTable("##fancy-ui-main-regions", column_count,
                        ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_BordersInnerV |
                            ImGuiTableFlags_SizingStretchProp,
                        ImVec2(0.0f, main_height))) {
    int column = 0;
    if (spec.activity_rail.visible && spec.activity_rail.draw) {
      ImGui::TableSetupColumn("Activity",
                              ImGuiTableColumnFlags_WidthFixed |
                                  ImGuiTableColumnFlags_NoResize,
                              kActivityRailWidth);
      ++column;
    }
    if (state.explorer_visible && spec.explorer.visible && spec.explorer.draw) {
      ImGui::TableSetupColumn("Explorer", ImGuiTableColumnFlags_WidthFixed,
                              state.explorer_width);
      ++column;
    }
    ImGui::TableSetupColumn("Workspace", ImGuiTableColumnFlags_WidthStretch,
                            1.0f);
    if (state.inspector_visible && spec.inspector.visible &&
        spec.inspector.draw) {
      ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthFixed,
                              state.inspector_width);
    }

    ImGui::TableNextRow();
    column = 0;
    if (spec.activity_rail.visible && spec.activity_rail.draw) {
      ImGui::TableSetColumnIndex(column++);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      DrawRegion(spec.activity_rail, ImVec2(0.0f, main_height));
      ImGui::PopStyleVar();
    }
    if (state.explorer_visible && spec.explorer.visible && spec.explorer.draw) {
      ImGui::TableSetColumnIndex(column);
      const float explorer_width = ImGui::GetContentRegionAvail().x;
      DrawRegion(spec.explorer, ImVec2(0.0f, main_height));
      result.state.explorer_width = explorer_width;
      ++column;
    }
    ImGui::TableSetColumnIndex(column++);
    DrawRegion(spec.workspace, ImVec2(0.0f, main_height));
    if (state.inspector_visible && spec.inspector.visible &&
        spec.inspector.draw) {
      ImGui::TableSetColumnIndex(column);
      const float inspector_width = ImGui::GetContentRegionAvail().x;
      DrawRegion(spec.inspector, ImVec2(0.0f, main_height));
      result.state.inspector_width = inspector_width;
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();

  if (state.operation_tray_visible) {
    DrawRegion(spec.operation_tray,
               ImVec2(0.0f, result.state.operation_tray_height));
  }
  DrawRegion(spec.operation_strip, ImVec2(0.0f, kOperationStripHeight));
  DrawRegion(spec.status_bar, ImVec2(0.0f, kStatusBarHeight));

  result.layout_changed =
      result.state.explorer_width != state.explorer_width ||
      result.state.inspector_width != state.inspector_width ||
      result.state.operation_tray_height != state.operation_tray_height;
  return result;
}

} // namespace fancy_ui::shell
