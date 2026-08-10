#include "fancy_ui/components/radio_group.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>

namespace fancy_ui {

RadioGroupResult RadioGroup(const RadioGroupSpec &spec) {
  RadioGroupResult result;
  if (spec.options.empty()) {
    return result;
  }
  const std::size_t selected =
      std::min(spec.selected_index, spec.options.size() - std::size_t{1});
  result.selected_index = selected;
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  ImGui::PushID(detail::Owned(spec.id).c_str());
  const detail::FieldLayout field = detail::BeginFieldLayout(spec.label);
  detail::BeginAvailability(spec.availability);
  for (std::size_t index = 0; index < spec.options.size(); ++index) {
    if (index > 0 && spec.layout == RadioGroupLayout::Horizontal) {
      ImGui::SameLine();
    }
    const SelectOption &option = spec.options[index];
    const bool enabled = option.enabled && option.availability.enabled &&
                         !option.availability.busy;
    ImGui::PushID(detail::Owned(option.id).c_str());
    ImGui::BeginDisabled(!enabled);
    const bool activated = ImGui::RadioButton(
        detail::Owned(option.label).c_str(), index == selected);
    const InteractionResult interaction = detail::CaptureInteraction();
    result.hovered = result.hovered || interaction.hovered;
    result.focused = result.focused || interaction.focused;
    result.active = result.active || interaction.active;
    if (interaction.focused) {
      int direction = 0;
      if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) ||
          ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) {
        direction = -1;
      } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false) ||
                 ImGui::IsKeyPressed(ImGuiKey_DownArrow, false)) {
        direction = 1;
      }
      std::size_t candidate = index;
      for (std::size_t tries = 0; direction != 0 && tries < spec.options.size();
           ++tries) {
        candidate =
            static_cast<std::size_t>((static_cast<int>(candidate) + direction +
                                      static_cast<int>(spec.options.size())) %
                                     static_cast<int>(spec.options.size()));
        const SelectOption &next = spec.options[candidate];
        if (next.enabled && next.availability.enabled &&
            !next.availability.busy) {
          result.changed = candidate != selected;
          result.selected_index = candidate;
          break;
        }
      }
    }
    if (activated && enabled) {
      result.changed = index != selected;
      result.selected_index = index;
    }
    detail::DrawFocusRing(interaction);
    if (interaction.hovered && !option.tooltip.empty()) {
      detail::ShowTooltip(option.tooltip);
    }
    ImGui::EndDisabled();
    ImGui::PopID();
  }
  detail::EndAvailability(spec.availability, {});
  detail::EndFieldLayout(field, {});
  ImGui::PopID();
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
