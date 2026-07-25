#include "fancy_ui/components/checkbox.hpp"

#include "internal/component_internal.hpp"

#include <imgui.h>

namespace fancy_ui {

CheckboxResult Checkbox(const CheckboxSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  bool value = spec.checked;

  ImGui::PushID(id.c_str());
  detail::BeginAvailability(spec.availability);
  const bool changed = ImGui::Checkbox(label.c_str(), &value);
  const InteractionResult interaction = detail::CaptureInteraction();
  detail::EndAvailability(spec.availability, spec.tooltip);
  ImGui::PopID();

  CheckboxResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = changed;
  result.value = value;
  return result;
}

} // namespace fancy_ui
