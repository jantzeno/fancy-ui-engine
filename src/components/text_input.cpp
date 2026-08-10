#include "fancy_ui/components/text_input.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <vector>

namespace fancy_ui {

TextInputResult TextInput(const TextInputSpec &spec) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  const std::string id = detail::Owned(spec.id);
  const std::size_t capacity = std::clamp<std::size_t>(spec.capacity, 2, 4096);
  std::vector<char> buffer(capacity, '\0');
  const std::size_t copy_length =
      std::min(spec.value.size(), buffer.size() - std::size_t{1});
  std::copy_n(spec.value.data(), copy_length, buffer.data());

  ImGui::PushID(id.c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::PushFieldControlState(spec.availability, spec.validation);
  detail::BeginAvailability(spec.availability);
  const std::string placeholder = detail::Owned(spec.placeholder);
  const bool changed = ImGui::InputTextWithHint("##value", placeholder.c_str(),
                                                buffer.data(), buffer.size());
  const InteractionResult interaction = detail::CaptureInteraction();
  const bool cancelled = detail::CancelledThisFrame(interaction);
  const bool committed = ImGui::IsItemDeactivatedAfterEdit() && !cancelled;
  detail::DrawFocusRing(interaction, spec.validation.invalid);
  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::PopFieldControlState(spec.availability, spec.validation);
  detail::EndFieldLayout(layout, spec.validation);
  ImGui::PopID();

  TextInputResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = changed && !cancelled;
  result.committed = committed;
  result.cancelled = cancelled;
  result.value =
      cancelled ? std::string(spec.value) : std::string(buffer.data());
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
