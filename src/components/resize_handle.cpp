#include "fancy_ui/components/resize_handle.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace fancy_ui {

float ClampResizeValue(const float value, const float minimum,
                       const float maximum) {
  const auto [low, high] = std::minmax(minimum, maximum);
  return std::clamp(value, low, high);
}

float ResizeValueAfterCommand(const float value, const float minimum,
                              const float maximum, const float step,
                              const ResizeCommand command) {
  switch (command) {
  case ResizeCommand::Decrease:
    return ClampResizeValue(value - std::abs(step), minimum, maximum);
  case ResizeCommand::Increase:
    return ClampResizeValue(value + std::abs(step), minimum, maximum);
  case ResizeCommand::Minimum:
    return std::min(minimum, maximum);
  case ResizeCommand::Maximum:
    return std::max(minimum, maximum);
  }
  return ClampResizeValue(value, minimum, maximum);
}

ResizeHandleResult ResizeHandle(const ResizeHandleSpec &spec) {
  ResizeHandleResult result;
  result.value = ClampResizeValue(spec.value, spec.minimum, spec.maximum);
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const bool vertical = spec.direction == ResizeDirection::Vertical;
  const ImVec2 size =
      vertical ? ImVec2(ImGui::GetContentRegionAvail().x, Scale(8.0f))
               : ImVec2(Scale(8.0f), metrics.geometry.compact_target);

  ImGui::PushID(detail::Owned(spec.id).c_str());
  ImGui::InvisibleButton("##resize-handle", size, ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  static_cast<InteractionResult &>(result) = interaction;
  if (interaction.active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    const ImVec2 delta = ImGui::GetIO().MouseDelta;
    const float movement = vertical ? -delta.y : delta.x;
    result.value = ClampResizeValue(result.value + movement / CurrentUiScale(),
                                    spec.minimum, spec.maximum);
    result.changed = result.value != spec.value;
  }
  if (interaction.hovered && spec.reset_value.has_value() &&
      ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    result.value =
        ClampResizeValue(*spec.reset_value, spec.minimum, spec.maximum);
    result.changed = result.value != spec.value;
  }

  std::optional<ResizeCommand> command;
  if (interaction.focused) {
    if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) {
      command = ResizeCommand::Minimum;
    } else if (ImGui::IsKeyPressed(ImGuiKey_End, false)) {
      command = ResizeCommand::Maximum;
    } else if (ImGui::IsKeyPressed(
                   vertical ? ImGuiKey_UpArrow : ImGuiKey_RightArrow, false)) {
      command = ResizeCommand::Increase;
    } else if (ImGui::IsKeyPressed(
                   vertical ? ImGuiKey_DownArrow : ImGuiKey_LeftArrow, false)) {
      command = ResizeCommand::Decrease;
    }
  }
  if (command.has_value()) {
    result.value = ResizeValueAfterCommand(
        result.value, spec.minimum, spec.maximum, spec.keyboard_step, *command);
    result.changed = result.value != spec.value;
  }

  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const SemanticPalette &palette = CurrentPalette();
  const ColorRgba color = interaction.active || interaction.hovered
                              ? palette.focus
                              : palette.border_strong;
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (vertical) {
    const float y = std::floor((minimum.y + maximum.y) * 0.5f);
    draw_list->AddLine(ImVec2(minimum.x, y), ImVec2(maximum.x, y),
                       ImGui::GetColorU32(ImVec4(color.red, color.green,
                                                 color.blue, color.alpha)),
                       Scale(2.0f));
  } else {
    const float x = std::floor((minimum.x + maximum.x) * 0.5f);
    draw_list->AddLine(ImVec2(x, minimum.y), ImVec2(x, maximum.y),
                       ImGui::GetColorU32(ImVec4(color.red, color.green,
                                                 color.blue, color.alpha)),
                       Scale(2.0f));
  }
  detail::DrawFocusRing(interaction, true);
  if ((interaction.hovered || interaction.focused) && !spec.tooltip.empty()) {
    detail::ShowTooltip(spec.tooltip);
  }
  ImGui::PopID();
  return result;
}

} // namespace fancy_ui
