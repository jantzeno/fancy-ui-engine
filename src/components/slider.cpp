#include "fancy_ui/components/slider.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <string>

namespace fancy_ui {

SliderResult Slider(const SliderSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  std::string format = detail::Owned(spec.format);
  if (!spec.unit.empty()) {
    format += " ";
    format += detail::Owned(spec.unit);
  }
  float value = spec.value;
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const SemanticPalette &palette = CurrentPalette();
  const auto to_imgui = [](const ColorRgba color) {
    return ImVec4(color.red, color.green, color.blue, color.alpha);
  };

  ImGui::PushID(id.c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::PushFieldControlState(spec.availability, spec.validation);
  if (disabled) {
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, to_imgui(palette.text_disabled));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,
                          to_imgui(palette.text_disabled));
  }
  detail::BeginAvailability(spec.availability);
  const bool changed = ImGui::SliderFloat("##value", &value, spec.minimum,
                                          spec.maximum, format.c_str());
  const InteractionResult interaction = detail::CaptureInteraction();
  const bool committed = ImGui::IsItemDeactivatedAfterEdit();
  detail::DrawFocusRing(interaction, true);
  detail::EndAvailability(spec.availability, spec.tooltip);
  if (disabled) {
    ImGui::PopStyleColor(2);
  }
  detail::PopFieldControlState(spec.availability, spec.validation);
  detail::EndFieldLayout(layout, spec.validation);
  ImGui::PopID();

  SliderResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = changed;
  result.committed = committed;
  result.value = value;
  return result;
}

} // namespace fancy_ui
