#include "fancy_ui/components/select.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace fancy_ui {

SelectResult Select(const SelectSpec &spec) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  const std::string id = detail::Owned(spec.id);
  const std::size_t selected =
      spec.options.empty()
          ? 0
          : std::min(spec.selected_index, spec.options.size() - std::size_t{1});
  std::size_t result_index = selected;
  bool changed = false;
  const std::string preview = spec.options.empty()
                                  ? std::string{}
                                  : detail::Owned(spec.options[selected].label);

  ImGui::PushID(id.c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::PushFieldControlState(spec.availability, spec.validation);
  detail::BeginAvailability(spec.availability);
  const bool open = ImGui::BeginCombo("##value", preview.c_str());
  const InteractionResult interaction = detail::CaptureInteraction();
  detail::DrawFocusRing(interaction, spec.validation.invalid);
  if (open) {
    for (std::size_t index = 0; index < spec.options.size(); ++index) {
      const SelectOption &option = spec.options[index];
      ImGui::PushID(detail::Owned(option.id).c_str());
      const bool option_enabled = option.enabled &&
                                  option.availability.enabled &&
                                  !option.availability.busy;
      ImGui::BeginDisabled(!option_enabled);
      if (ImGui::Selectable(detail::Owned(option.label).c_str(),
                            index == selected) &&
          option_enabled) {
        changed = true;
        result_index = index;
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        const std::string_view tooltip =
            option_enabled || option.availability.reason.empty()
                ? option.tooltip
                : option.availability.reason;
        if (!tooltip.empty()) {
          detail::ShowTooltip(tooltip);
        }
      }
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::EndCombo();
  }
  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::PopFieldControlState(spec.availability, spec.validation);
  detail::EndFieldLayout(layout, spec.validation);
  ImGui::PopID();

  SelectResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed =
      changed && spec.availability.enabled && !spec.availability.busy;
  result.selected_index = result.changed ? result_index : selected;
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
