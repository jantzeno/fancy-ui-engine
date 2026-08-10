#include "fancy_ui/components/duration_input.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace fancy_ui {

DurationResult Duration(const DurationSpec &spec) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  const std::string id = detail::Owned(spec.id);
  int hours = std::clamp(spec.hours, 0, 23);
  int minutes = std::clamp(spec.minutes, 0, 59);
  bool changed = false;
  bool committed = false;
  bool cancelled = false;

  ImGui::PushID(id.c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::PushFieldControlState(spec.availability, spec.validation);
  detail::BeginAvailability(spec.availability);
  const float available = ImGui::GetContentRegionAvail().x;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float gap = metrics.spacing.space03;
  const float field_width =
      std::max(Scale(88.0f), std::floor((available - gap) * 0.5f));
  ImGui::SetNextItemWidth(field_width);
  changed |= ImGui::InputScalar("##hours", ImGuiDataType_S32, &hours, nullptr,
                                nullptr, "%d Hours");
  const InteractionResult hours_interaction = detail::CaptureInteraction();
  cancelled |= detail::CancelledThisFrame(hours_interaction);
  committed |= ImGui::IsItemDeactivatedAfterEdit();
  detail::DrawFocusRing(hours_interaction);
  ImGui::SameLine(0.0f, gap);
  ImGui::SetNextItemWidth(field_width);
  changed |= ImGui::InputScalar("##minutes", ImGuiDataType_S32, &minutes,
                                nullptr, nullptr, "%d Minutes");
  const InteractionResult minutes_interaction = detail::CaptureInteraction();
  cancelled |= detail::CancelledThisFrame(minutes_interaction);
  committed |= ImGui::IsItemDeactivatedAfterEdit();
  detail::DrawFocusRing(minutes_interaction);
  const InteractionResult combined{
      .hovered = hours_interaction.hovered || minutes_interaction.hovered,
      .focused = hours_interaction.focused || minutes_interaction.focused,
      .active = hours_interaction.active || minutes_interaction.active,
  };
  if ((hours_interaction.hovered || hours_interaction.focused) &&
      !spec.tooltip.empty()) {
    detail::ShowTooltip(spec.tooltip);
  }
  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::PopFieldControlState(spec.availability, spec.validation);
  detail::EndFieldLayout(layout, spec.validation);
  ImGui::PopID();

  hours = std::clamp(hours, 0, 23);
  minutes = std::clamp(minutes, 0, 59);
  DurationResult result;
  static_cast<InteractionResult &>(result) = combined;
  result.changed = changed && !cancelled;
  result.committed = committed && !cancelled;
  result.cancelled = cancelled;
  result.hours = cancelled ? spec.hours : hours;
  result.minutes = cancelled ? spec.minutes : minutes;
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
