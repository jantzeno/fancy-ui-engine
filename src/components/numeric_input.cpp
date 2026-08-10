#include "fancy_ui/components/numeric_input.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace fancy_ui {

NumericInputResult NumericInput(const NumericInputSpec &spec) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  const std::string id = detail::Owned(spec.id);
  std::string format = detail::Owned(spec.format);
  if (!spec.unit.empty()) {
    format += " ";
    format += detail::Owned(spec.unit);
  }
  double value = spec.value;

  ImGui::PushID(id.c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::PushFieldControlState(spec.availability, spec.validation);
  detail::BeginAvailability(spec.availability);
  const bool changed =
      ImGui::InputDouble("##value", &value, 0.0, 0.0, format.c_str());
  if (spec.minimum.has_value()) {
    value = std::max(value, *spec.minimum);
  }
  if (spec.maximum.has_value()) {
    value = std::min(value, *spec.maximum);
  }
  const InteractionResult interaction = detail::CaptureInteraction();
  const bool cancelled = detail::CancelledThisFrame(interaction);
  const bool committed = ImGui::IsItemDeactivatedAfterEdit() && !cancelled;
  detail::DrawFocusRing(interaction, spec.validation.invalid);
  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::PopFieldControlState(spec.availability, spec.validation);
  detail::EndFieldLayout(layout, spec.validation);
  ImGui::PopID();

  NumericInputResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = changed && !cancelled;
  result.committed = committed;
  result.cancelled = cancelled;
  result.value = cancelled ? spec.value : value;
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
