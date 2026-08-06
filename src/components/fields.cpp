#include "fancy_ui/components/fields.hpp"

#include "fancy_ui/components/button.hpp"
#include "fancy_ui/components/checkbox.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
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

ColorRgba ClampColor(const ColorRgba color) {
  return {
      .red = std::clamp(color.red, 0.0f, 1.0f),
      .green = std::clamp(color.green, 0.0f, 1.0f),
      .blue = std::clamp(color.blue, 0.0f, 1.0f),
      .alpha = std::clamp(color.alpha, 0.0f, 1.0f),
  };
}

float ColorPickerPopupWidth(const ColorPickerPopupSpec &spec) {
  const ImGuiStyle &style = ImGui::GetStyle();
  const float picker_width = Scale(260.0f);
  float content_width = picker_width;
  if (spec.layout == ColorPickerLayout::CurrentAndOriginal) {
    content_width += style.ItemInnerSpacing.x + ImGui::GetFrameHeight() * 3.0f;
  }
  content_width = std::max(
      content_width, ImGui::CalcTextSize(detail::Owned(spec.title).c_str()).x);
  content_width =
      std::max(content_width, Scale(72.0f * 2.0f) + style.ItemSpacing.x);
  return content_width + style.WindowPadding.x * 2.0f;
}

} // namespace

NumericInputResult NumericInput(const NumericInputSpec &spec) {
  ImGui::PushFont(nullptr, Scale(21.0f));
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
  ImGui::PopFont();
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
  ImGui::PushFont(nullptr, Scale(21.0f));
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
  ImGui::PopFont();
  return result;
}

DurationResult Duration(const DurationSpec &spec) {
  ImGui::PushFont(nullptr, Scale(21.0f));
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
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float gap = metrics.spacing.space03;
  const float field_width =
      std::max(Scale(88.0f), std::floor((available - gap) * 0.5f));
  ImGui::SetNextItemWidth(field_width);
  changed |= ImGui::InputScalar("##hours", ImGuiDataType_S32, &hours, nullptr,
                                nullptr, "%d Hours");
  InteractionResult hours_interaction = detail::CaptureInteraction();
  cancelled |= CancelledThisFrame(hours_interaction);
  committed |= ImGui::IsItemDeactivatedAfterEdit();
  detail::DrawFocusRing(hours_interaction);
  ImGui::SameLine(0.0f, gap);
  ImGui::SetNextItemWidth(field_width);
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

ColorPickerPopupResult ColorPickerPopup(const ColorPickerPopupSpec &spec,
                                        ColorPickerState &state) {
  ColorPickerPopupResult result;
  result.value = spec.value;
  const bool was_editing = state.editing;

  ImGui::PushID(detail::Owned(spec.id).c_str());
  if (spec.request_open) {
    state.editing = true;
    state.restore_focus = false;
    state.original = ClampColor(spec.value);
    state.draft = state.original;
    ImGui::OpenPopup("##color-picker");
    result.opened = true;
  }

  const float popup_width = ColorPickerPopupWidth(spec);
  ImGui::SetNextWindowSizeConstraints(ImVec2(popup_width, 0.0f),
                                      ImVec2(popup_width, FLT_MAX));
  if (ImGui::BeginPopup("##color-picker")) {
    state.editing = true;
    ImGui::TextUnformatted(detail::Owned(spec.title).c_str());
    ImGui::Separator();
    std::array<float, 4> draft{
        state.draft.red,
        state.draft.green,
        state.draft.blue,
        state.draft.alpha,
    };
    const std::array<float, 4> original{
        state.original.red,
        state.original.green,
        state.original.blue,
        state.original.alpha,
    };
    ImGuiColorEditFlags flags =
        ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoLabel;
    if (spec.layout == ColorPickerLayout::Compact) {
      flags |= ImGuiColorEditFlags_NoSidePreview;
    }
    if (spec.show_alpha) {
      flags |= ImGuiColorEditFlags_AlphaBar;
    } else {
      flags |= ImGuiColorEditFlags_NoAlpha;
    }
    ImGui::SetNextItemWidth(Scale(260.0f));
    if (ImGui::ColorPicker4("##value", draft.data(), flags, original.data())) {
      state.draft = ClampColor({
          .red = draft[0],
          .green = draft[1],
          .blue = draft[2],
          .alpha = spec.show_alpha ? draft[3] : state.original.alpha,
      });
    }

    const bool window_focused =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    bool commit = window_focused && ImGui::IsKeyPressed(ImGuiKey_Enter, false);
    bool cancel = window_focused && ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    commit |= Button({
                         .id = "apply",
                         .label = "Apply",
                         .variant = ButtonVariant::Primary,
                         .size = {.x = 72.0f, .y = 28.0f},
                     })
                  .activated;
    ImGui::SameLine();
    cancel |= Button({
                         .id = "cancel",
                         .label = "Cancel",
                         .size = {.x = 72.0f, .y = 28.0f},
                     })
                  .activated;

    if (cancel) {
      result.cancelled = true;
      result.value = state.original;
      state.editing = false;
      state.restore_focus = true;
      ImGui::CloseCurrentPopup();
    } else if (commit) {
      result.changed = state.draft != state.original;
      result.committed = true;
      result.value = state.draft;
      state.editing = false;
      state.restore_focus = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  } else if (was_editing && !spec.request_open && state.editing) {
    // Clicking outside a non-modal picker is equivalent to cancelling it.
    result.cancelled = true;
    result.value = state.original;
    state.editing = false;
    state.restore_focus = true;
  }
  result.picker_open = state.editing;
  ImGui::PopID();
  return result;
}

ColorSwatchResult ColorSwatch(const ColorSwatchSpec &spec,
                              ColorPickerState &state) {
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
  if (state.restore_focus) {
    ImGui::SetKeyboardFocusHere(-1);
    state.restore_focus = false;
  }
  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::EndFieldLayout(layout, {});

  const bool request_open =
      activated && spec.availability.enabled && !spec.availability.busy;
  const ColorPickerPopupResult picker = ColorPickerPopup(
      {
          .id = "picker",
          .title = spec.picker_title,
          .value = spec.value,
          .request_open = request_open,
          .show_alpha = spec.show_alpha,
          .layout = spec.picker_layout,
      },
      state);
  ImGui::PopID();

  ColorSwatchResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = request_open;
  result.changed = picker.changed;
  result.committed = picker.committed;
  result.cancelled = picker.cancelled;
  result.picker_open = picker.picker_open;
  result.value = picker.value;
  return result;
}

} // namespace fancy_ui
