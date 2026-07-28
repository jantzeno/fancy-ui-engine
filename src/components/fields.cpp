#include "fancy_ui/components/fields.hpp"

#include "fancy_ui/components/checkbox.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

bool CancelledThisFrame(const InteractionResult &interaction) {
  return (interaction.active || interaction.focused) &&
         ImGui::IsKeyPressed(ImGuiKey_Escape, false);
}

} // namespace

NumericInputResult NumericInput(const NumericInputSpec &spec) {
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
  const bool cancelled = CancelledThisFrame(interaction);
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
  return result;
}

TextInputResult TextInput(const TextInputSpec &spec) {
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
  const bool changed =
      ImGui::InputText("##value", buffer.data(), buffer.size());
  const InteractionResult interaction = detail::CaptureInteraction();
  const bool cancelled = CancelledThisFrame(interaction);
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
  return result;
}

SelectResult Select(const SelectSpec &spec) {
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
      ImGui::BeginDisabled(!option.enabled);
      if (ImGui::Selectable(detail::Owned(option.label).c_str(),
                            index == selected, 0, ImVec2(0.0f, Scale(24.0f))) &&
          option.enabled) {
        changed = true;
        result_index = index;
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
  return result;
}

DurationResult Duration(const DurationSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  int hours = std::clamp(spec.hours, 0, 23);
  int minutes = std::clamp(spec.minutes, 0, 59);
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  InteractionResult combined;

  ImGui::PushID(id.c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::PushFieldControlState(spec.availability, spec.validation);
  detail::BeginAvailability(spec.availability);
  const float available = ImGui::GetContentRegionAvail().x;
  const float gap = Scale(6.0f);
  ImGui::SetNextItemWidth(std::max(Scale(72.0f), (available - gap) * 0.5f));
  changed |= ImGui::InputScalar("##hours", ImGuiDataType_S32, &hours, nullptr,
                                nullptr, "%d Hours");
  InteractionResult hours_interaction = detail::CaptureInteraction();
  cancelled |= CancelledThisFrame(hours_interaction);
  committed |= ImGui::IsItemDeactivatedAfterEdit();
  detail::DrawFocusRing(hours_interaction);
  ImGui::SameLine(0.0f, gap);
  ImGui::SetNextItemWidth(std::max(Scale(88.0f), (available - gap) * 0.5f));
  changed |= ImGui::InputScalar("##minutes", ImGuiDataType_S32, &minutes,
                                nullptr, nullptr, "%d Minutes");
  InteractionResult minutes_interaction = detail::CaptureInteraction();
  cancelled |= CancelledThisFrame(minutes_interaction);
  committed |= ImGui::IsItemDeactivatedAfterEdit();
  detail::DrawFocusRing(minutes_interaction);
  combined = {
      .hovered = hours_interaction.hovered || minutes_interaction.hovered,
      .focused = hours_interaction.focused || minutes_interaction.focused,
      .active = hours_interaction.active || minutes_interaction.active,
  };
  if ((hours_interaction.hovered || hours_interaction.focused) &&
      !spec.tooltip.empty()) {
    ImGui::SetTooltip("%s", detail::Owned(spec.tooltip).c_str());
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
  return result;
}

VisibilityToggleResult VisibilityToggle(const VisibilityToggleSpec &spec) {
  ImGui::PushID(detail::Owned(spec.id).c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  const std::string state_label = spec.state == ToggleState::On    ? "Visible"
                                  : spec.state == ToggleState::Off ? "Hidden"
                                                                   : "Mixed";
  const CheckboxResult checkbox = Checkbox({
      .id = "value",
      .label = state_label,
      .tooltip = spec.tooltip,
      .state = spec.state,
      .on_icon = spec.visible_icon,
      .off_icon = spec.hidden_icon,
      .show_checkbox = true,
      .availability = spec.availability,
  });
  detail::EndFieldLayout(layout, {});
  ImGui::PopID();

  VisibilityToggleResult result;
  static_cast<InteractionResult &>(result) = checkbox;
  result.changed = checkbox.changed;
  result.state = checkbox.state;
  return result;
}

ColorSwatchResult ColorSwatch(const ColorSwatchSpec &spec) {
  ImGui::PushID(detail::Owned(spec.id).c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::BeginAvailability(spec.availability);
  const ImVec2 size(Scale(32.0f), Scale(32.0f));
  const bool activated =
      ImGui::InvisibleButton("##swatch", size, ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const SemanticPalette &palette = CurrentPalette();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(minimum, maximum,
                           ImGui::GetColorU32(ToImVec4(palette.surface_raised)),
                           Scale(3.0f));
  draw_list->AddRect(minimum, maximum,
                     ImGui::GetColorU32(ToImVec4(palette.border_strong)),
                     Scale(3.0f), ImDrawFlags_RoundCornersAll, Scale(1.0f));
  const ImVec2 inset(Scale(4.0f), Scale(4.0f));
  const ImVec2 inner_min(minimum.x + inset.x, minimum.y + inset.y);
  const ImVec2 inner_max(maximum.x - inset.x, maximum.y - inset.y);
  const std::size_t count = std::max<std::size_t>(spec.colors.size(), 1);
  for (std::size_t index = 0; index < count; ++index) {
    const ColorRgba color =
        spec.colors.empty() ? palette.surface_muted : spec.colors[index];
    const float left = inner_min.x + (inner_max.x - inner_min.x) *
                                         static_cast<float>(index) /
                                         static_cast<float>(count);
    const float right = inner_min.x + (inner_max.x - inner_min.x) *
                                          static_cast<float>(index + 1) /
                                          static_cast<float>(count);
    draw_list->AddRectFilled(ImVec2(left, inner_min.y),
                             ImVec2(right, inner_max.y),
                             ImGui::GetColorU32(ToImVec4(color)));
  }
  detail::DrawFocusRing(interaction, true);
  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::EndFieldLayout(layout, {});
  ImGui::PopID();

  ColorSwatchResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated =
      activated && spec.availability.enabled && !spec.availability.busy;
  return result;
}

} // namespace fancy_ui
