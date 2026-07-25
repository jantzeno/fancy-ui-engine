#include "fancy_ui/components/slider.hpp"

#include "internal/component_internal.hpp"

#include <imgui.h>

#include <string>

namespace fancy_ui {

SliderResult Slider(const SliderSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  std::string format = detail::Owned(spec.format);
  if (!spec.unit.empty()) {
    format += " ";
    format += detail::Owned(spec.unit);
  }
  float value = spec.value;

  ImGui::PushID(id.c_str());
  detail::BeginAvailability(spec.availability);
  const bool changed = ImGui::SliderFloat(label.c_str(), &value, spec.minimum,
                                          spec.maximum, format.c_str());
  const InteractionResult interaction = detail::CaptureInteraction();
  const bool committed = ImGui::IsItemDeactivatedAfterEdit();
  detail::EndAvailability(spec.availability, spec.tooltip);
  ImGui::PopID();

  SliderResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = changed;
  result.committed = committed;
  result.value = value;
  return result;
}

} // namespace fancy_ui
