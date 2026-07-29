#include "fancy_ui/components/navigation.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <cmath>
#include <string>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

NavigationItemResult NavigationItem(const NavigationItemSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float target_size = metrics.shell.activity_rail_width;
  const float icon_size = metrics.geometry.activity_icon;
  const SemanticPalette &palette = CurrentPalette();

  ImGui::PushID(id.c_str());
  detail::BeginAvailability(spec.availability);
  const bool activated = ImGui::InvisibleButton(
      "##navigation-item", ImVec2(target_size, target_size),
      ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const bool keyboard_focused =
      interaction.focused && ImGui::GetIO().NavVisible;
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImGui::EndDisabled();

  ColorRgba fill = palette.application_surface;
  fill.alpha = 0.0f;
  ColorRgba foreground = palette.text_secondary;
  if (disabled) {
    foreground = palette.text_disabled;
  } else if (spec.selected) {
    fill = palette.selection;
    foreground = palette.focus;
  } else if (interaction.active) {
    fill = palette.control_pressed;
    foreground = palette.text_primary;
  } else if (interaction.hovered) {
    fill = palette.control_hover;
    foreground = palette.text_primary;
  }

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (fill.alpha > 0.0f) {
    draw_list->AddRectFilled(minimum, maximum,
                             ImGui::GetColorU32(ToImVec4(fill)));
  }
  if (spec.selected) {
    const float cue_height = Scale(28.0f);
    const float center_y = (minimum.y + maximum.y) * 0.5f;
    draw_list->AddRectFilled(
        ImVec2(minimum.x, center_y - cue_height * 0.5f),
        ImVec2(minimum.x + Scale(3.0f), center_y + cue_height * 0.5f),
        ImGui::GetColorU32(ToImVec4(palette.focus)), Scale(1.5f),
        ImDrawFlags_RoundCornersRight);
  }
  if (spec.draw_icon) {
    const float left = std::floor((minimum.x + maximum.x - icon_size) * 0.5f);
    const float top = std::floor((minimum.y + maximum.y - icon_size) * 0.5f);
    spec.draw_icon(
        Rect{.minimum = {.x = left, .y = top},
             .maximum = {.x = left + icon_size, .y = top + icon_size}},
        foreground);
  }
  if (keyboard_focused) {
    draw_list->AddRect(ImVec2(minimum.x + 3.0f, minimum.y + 3.0f),
                       ImVec2(maximum.x - 3.0f, maximum.y - 3.0f),
                       ImGui::GetColorU32(ToImVec4(palette.focus)), 3.0f,
                       ImDrawFlags_RoundCornersAll, 2.0f);
  }

  if (interaction.hovered || keyboard_focused) {
    std::string tooltip =
        detail::Owned(spec.tooltip.empty() ? spec.label : spec.tooltip);
    if (disabled && !spec.availability.reason.empty()) {
      if (!tooltip.empty()) {
        tooltip += "\n";
      }
      tooltip += detail::Owned(spec.availability.reason);
    }
    if (!tooltip.empty()) {
      detail::ShowTooltip(tooltip);
    }
  }
  ImGui::PopID();

  NavigationItemResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = activated && !disabled;
  return result;
}

} // namespace fancy_ui
