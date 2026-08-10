#include "internal/component_internal.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"

#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace fancy_ui::detail {

namespace {

thread_local std::optional<InteractionPreview> interaction_preview;
thread_local std::optional<float> field_layout_preview_label_width;

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

void PushInvisibleSliderStyle() {
  constexpr ImVec4 transparent{};
  ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, transparent);
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, transparent);
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, transparent);
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, transparent);
  ImGui::PushStyleColor(ImGuiCol_Text, transparent);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
}

void PopInvisibleSliderStyle() {
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(6);
}

template <typename Value>
std::string FormatSliderValue(const Value value, const ImGuiDataType data_type,
                              const std::string_view format,
                              const std::string_view unit) {
  char buffer[64]{};
  const std::string owned_format = Owned(format);
  ImGui::DataTypeFormatString(buffer, sizeof(buffer), data_type, &value,
                              owned_format.c_str());
  std::string text(buffer);
  if (!unit.empty()) {
    if (unit != "%") {
      text += ' ';
    }
    text += unit;
  }
  return text;
}

struct SliderGeometry {
  ImVec2 minimum;
  ImVec2 maximum;
  float thumb_width;
  float thumb_height;
  float track_minimum_x;
  float track_maximum_x;
  float track_y;
};

SliderGeometry ResolveSliderGeometry(const std::string_view output,
                                     const bool framed) {
  constexpr float grab_padding = 2.0f;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const float thumb_width = metrics.geometry.icon;
  const float horizontal_padding = framed ? metrics.spacing.space03 : 0.0f;
  const float output_width =
      output.empty()
          ? 0.0f
          : ImGui::CalcTextSize(output.data(), output.data() + output.size()).x;
  const float output_start = maximum.x - horizontal_padding - output_width;
  const float thumb_inset = grab_padding + thumb_width * 0.5f;
  const float track_minimum_x = minimum.x + horizontal_padding + thumb_inset;
  const float track_maximum_x =
      std::max(track_minimum_x,
               (output.empty() ? maximum.x - horizontal_padding
                               : output_start - metrics.spacing.space03) -
                   thumb_inset);
  return {
      .minimum = minimum,
      .maximum = maximum,
      .thumb_width = thumb_width,
      .thumb_height = metrics.geometry.compact_target - metrics.spacing.space02,
      .track_minimum_x = track_minimum_x,
      .track_maximum_x = track_maximum_x,
      .track_y = (minimum.y + maximum.y) * 0.5f,
  };
}

template <typename Value>
bool RetargetMouseSlider(const ImGuiID item_id, Value &value,
                         const Value original_value,
                         const ImGuiDataType data_type, const Value minimum,
                         const Value maximum, const char *format,
                         const ImGuiSliderFlags flags,
                         const SliderGeometry &geometry) {
  ImGuiContext &context = *GImGui;
  if (context.ActiveId != item_id ||
      context.ActiveIdSource != ImGuiInputSource_Mouse ||
      !context.IO.MouseDown[0] || ImGui::TempInputIsActive(item_id)) {
    return value != original_value;
  }

  constexpr float grab_padding = 2.0f;
  const float behavior_inset = grab_padding + geometry.thumb_width * 0.5f;
  const ImRect behavior_bounds(
      ImVec2(geometry.track_minimum_x - behavior_inset, geometry.minimum.y),
      ImVec2(geometry.track_maximum_x + behavior_inset, geometry.maximum.y));
  ImRect grab_bounds;
  value = original_value;
  const bool changed =
      ImGui::SliderBehavior(behavior_bounds, item_id, data_type, &value,
                            &minimum, &maximum, format, flags, &grab_bounds);
  if (changed) {
    ImGui::MarkItemEdited(item_id);
  }
  return value != original_value;
}

void DrawSliderPresentation(const float normalized_value,
                            const std::string_view output, const bool framed) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const SliderGeometry geometry = ResolveSliderGeometry(output, framed);
  const bool active = ImGui::IsItemActive();
  const bool hovered = ImGui::IsItemHovered();
  const ImVec4 frame_color =
      ImGui::GetStyleColorVec4(active    ? ImGuiCol_FrameBgActive
                               : hovered ? ImGuiCol_FrameBgHovered
                                         : ImGuiCol_FrameBg);
  const ImVec4 grab_color = ImGui::GetStyleColorVec4(
      active ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  if (framed) {
    draw_list->AddRectFilled(geometry.minimum, geometry.maximum,
                             ImGui::GetColorU32(frame_color),
                             metrics.geometry.control_radius);
    draw_list->AddRect(
        geometry.minimum, geometry.maximum, ImGui::GetColorU32(ImGuiCol_Border),
        metrics.geometry.control_radius, 0, metrics.geometry.border);
  }

  const float track_radius = metrics.spacing.space02 * 0.5f;
  const ImVec2 track_minimum(geometry.track_minimum_x,
                             geometry.track_y - metrics.spacing.space02 * 0.5f);
  const ImVec2 track_maximum(geometry.track_maximum_x,
                             geometry.track_y + metrics.spacing.space02 * 0.5f);
  draw_list->AddRectFilled(track_minimum, track_maximum,
                           ImGui::GetColorU32(ToImVec4(palette.surface_raised)),
                           track_radius);

  const float thumb_x = geometry.track_minimum_x +
                        (geometry.track_maximum_x - geometry.track_minimum_x) *
                            std::clamp(normalized_value, 0.0f, 1.0f);
  if (thumb_x > geometry.track_minimum_x) {
    draw_list->AddRectFilled(track_minimum, ImVec2(thumb_x, track_maximum.y),
                             ImGui::GetColorU32(grab_color), track_radius);
  }
  draw_list->AddRect(track_minimum, track_maximum,
                     ImGui::GetColorU32(ToImVec4(palette.border_strong)),
                     track_radius, ImDrawFlags_RoundCornersAll,
                     metrics.geometry.border);
  const ImVec2 thumb_minimum(thumb_x - geometry.thumb_width * 0.5f,
                             geometry.track_y - geometry.thumb_height * 0.5f);
  const ImVec2 thumb_maximum(thumb_x + geometry.thumb_width * 0.5f,
                             geometry.track_y + geometry.thumb_height * 0.5f);
  draw_list->AddRectFilled(thumb_minimum, thumb_maximum,
                           ImGui::GetColorU32(frame_color),
                           metrics.geometry.control_radius);
  draw_list->AddRect(
      thumb_minimum, thumb_maximum, ImGui::GetColorU32(grab_color),
      metrics.geometry.control_radius, 0, metrics.geometry.focus_ring);

  if (!output.empty()) {
    const ImVec2 text_size =
        ImGui::CalcTextSize(output.data(), output.data() + output.size());
    draw_list->AddText(ImVec2(geometry.maximum.x -
                                  (framed ? metrics.spacing.space03 : 0.0f) -
                                  text_size.x,
                              geometry.track_y - text_size.y * 0.5f),
                       ImGui::GetColorU32(ImGuiCol_Text), output.data(),
                       output.data() + output.size());
  }
}

} // namespace

ScopedInteractionPreview::ScopedInteractionPreview(
    const InteractionPreview preview)
    : previous_(interaction_preview) {
  interaction_preview = preview;
}

ScopedInteractionPreview::~ScopedInteractionPreview() {
  interaction_preview = previous_;
}

ScopedFieldLayoutPreview::ScopedFieldLayoutPreview(const float label_width)
    : previous_label_width_(field_layout_preview_label_width) {
  field_layout_preview_label_width = std::max(0.0f, label_width);
}

ScopedFieldLayoutPreview::~ScopedFieldLayoutPreview() {
  field_layout_preview_label_width = previous_label_width_;
}

std::string Owned(const std::string_view value) {
  return std::string(value.data(), value.size());
}

std::string EllipsizeText(const std::string_view text, const float width) {
  const std::string owned = Owned(text);
  if (width <= 0.0f) {
    return {};
  }
  if (ImGui::CalcTextSize(owned.c_str()).x <= width) {
    return owned;
  }
  constexpr std::string_view suffix = "...";
  const float suffix_width = ImGui::CalcTextSize(suffix.data()).x;
  std::size_t length = owned.size();
  while (length > 0) {
    --length;
    while (length > 0 &&
           (static_cast<unsigned char>(owned[length]) & 0xc0U) == 0x80U) {
      --length;
    }
    const std::string candidate = owned.substr(0, length);
    if (ImGui::CalcTextSize(candidate.c_str()).x + suffix_width <= width) {
      return candidate + std::string(suffix);
    }
  }
  return width >= suffix_width ? std::string(suffix) : std::string{};
}

void ShowTooltip(const std::string_view text) {
  const std::string owned_text = Owned(text);
  const ImVec2 padding(Scale(8.0f), Scale(8.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
  ImGui::SetTooltip("%s", owned_text.c_str());
  ImGui::PopStyleVar();
}

void DrawSecondaryText(const std::string_view text) {
  const ColorRgba color = CurrentPalette().text_secondary;
  ImGui::TextColored(ToImVec4(color), "%.*s", static_cast<int>(text.size()),
                     text.data());
}

void DrawSecondaryTextWrapped(const std::string_view text) {
  ImGui::PushStyleColor(ImGuiCol_Text,
                        ToImVec4(CurrentPalette().text_secondary));
  ImGui::TextWrapped("%.*s", static_cast<int>(text.size()), text.data());
  ImGui::PopStyleColor();
}

float ResolveButtonVerticalPadding(const float requested_height,
                                   const float text_height,
                                   const float default_padding) {
  if (requested_height <= 0.0f) {
    return default_padding;
  }
  const float centered_padding =
      std::floor(std::max(0.0f, (requested_height - text_height) * 0.5f));
  return std::min(default_padding, centered_padding);
}

FieldLayout BeginFieldLayout(const std::string_view label) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const std::string owned_label = Owned(label);
  if (!field_layout_preview_label_width.has_value() &&
      ImGui::GetContentRegionAvail().x < metrics.inspector.stack_breakpoint) {
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_secondary));
    ImGui::TextUnformatted(owned_label.c_str());
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(-std::numeric_limits<float>::min());
    return {};
  }

  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
  const bool table = ImGui::BeginTable("##field-layout", 3,
                                       ImGuiTableFlags_SizingStretchProp |
                                           ImGuiTableFlags_NoSavedSettings);
  if (table) {
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed,
                            field_layout_preview_label_width.value_or(
                                metrics.inspector.label_width));
    ImGui::TableSetupColumn("gap", ImGuiTableColumnFlags_WidthFixed,
                            metrics.spacing.space03);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_secondary));
    ImGui::TextUnformatted(owned_label.c_str());
    ImGui::PopStyleColor();
    ImGui::TableSetColumnIndex(2);
    ImGui::SetNextItemWidth(-std::numeric_limits<float>::min());
  }
  return {.table = table, .cell_padding_pushed = true};
}

void EndFieldLayout(const FieldLayout layout, const Validation &validation) {
  if (layout.table) {
    ImGui::EndTable();
  }
  if (layout.cell_padding_pushed) {
    ImGui::PopStyleVar();
  }
  DrawValidationHint(validation);
}

void PushFieldControlState(const Availability &availability,
                           const Validation &validation) {
  const bool disabled = !availability.enabled || availability.busy;
  const SemanticPalette &palette = CurrentPalette();
  if (disabled) {
    ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(palette.text_disabled));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,
                          ToImVec4(palette.control_disabled_fill));
    ImGui::PushStyleColor(ImGuiCol_Border,
                          ToImVec4(palette.control_disabled_border));
  } else if (validation.invalid) {
    ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(palette.failure));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(palette.border_strong));
  }
}

void PopFieldControlState(const Availability &availability,
                          const Validation &validation) {
  if (!availability.enabled || availability.busy) {
    ImGui::PopStyleColor(3);
  } else {
    ImGui::PopStyleColor();
  }
}

void BeginAvailability(const Availability &availability) {
  ImGui::BeginDisabled(!availability.enabled || availability.busy);
}

void EndAvailability(const Availability &availability,
                     const std::string_view tooltip) {
  const bool hovered =
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
  const bool keyboard_focused =
      ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
  ImGui::EndDisabled();
  if (!hovered && !keyboard_focused) {
    return;
  }

  if ((!availability.enabled || availability.busy) &&
      !availability.reason.empty()) {
    ShowTooltip(availability.reason);
  } else if (!tooltip.empty()) {
    ShowTooltip(tooltip);
  }
}

InteractionResult CaptureInteraction() {
  if (interaction_preview.has_value()) {
    switch (*interaction_preview) {
    case InteractionPreview::Rest:
      return {};
    case InteractionPreview::Hovered:
      return {.hovered = true};
    case InteractionPreview::Pressed:
      return {.hovered = true, .active = true};
    case InteractionPreview::Focused:
      return {.focused = true};
    }
  }
  return {
      .hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
      .focused = ImGui::IsItemFocused(),
      .active = ImGui::IsItemActive(),
  };
}

bool CancelledThisFrame(const InteractionResult &interaction) {
  return (interaction.active || interaction.focused) &&
         ImGui::IsKeyPressed(ImGuiKey_Escape, false);
}

std::optional<InteractionPreview> CurrentInteractionPreview() {
  return interaction_preview;
}

ControlColors ResolveControlColors(const ControlState &state) {
  const SemanticPalette &palette = CurrentPalette();
  ControlColors colors{
      .fill = palette.control,
      .border = palette.border_strong,
      .text = palette.text_primary,
  };

  if (state.primary) {
    colors.fill = palette.action_primary;
    colors.border = palette.action_primary;
    colors.text = palette.on_emphasis;
  } else if (state.tertiary) {
    colors.fill.alpha = 0.0f;
    colors.border.alpha = 0.0f;
  } else if (state.destructive) {
    colors.border = palette.failure;
    colors.text = palette.text_primary;
  }

  if (state.hovered && !state.selected) {
    colors.fill =
        state.primary ? palette.action_primary_hover : palette.control_hover;
  }
  if (state.pressed && !state.selected) {
    colors.fill = state.primary ? palette.action_primary_pressed
                                : palette.control_pressed;
  }
  if (state.selected) {
    colors.fill = palette.selection;
    colors.border = palette.focus;
    colors.text = palette.focus;
  }
  if (state.invalid) {
    colors.border = palette.failure;
  }
  if (state.disabled) {
    colors.fill = palette.control_disabled_fill;
    colors.border = palette.control_disabled_border;
    colors.text = palette.text_disabled;
  }
  return colors;
}

void DrawFocusRing(const InteractionResult &interaction,
                   const bool high_contrast_separator, const float rounding) {
  const bool preview_focused =
      interaction_preview == InteractionPreview::Focused;
  if (!interaction.focused ||
      (!preview_focused && !ImGui::GetIO().NavVisible)) {
    return;
  }

  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (high_contrast_separator) {
    draw_list->AddRect(ImVec2(minimum.x - Scale(0.5f), minimum.y - Scale(0.5f)),
                       ImVec2(maximum.x + Scale(0.5f), maximum.y + Scale(0.5f)),
                       ImGui::GetColorU32(ToImVec4(CurrentPalette().surface)),
                       Scale(rounding + 0.5f), ImDrawFlags_RoundCornersAll,
                       Scale(1.0f));
  }
  draw_list->AddRect(ImVec2(minimum.x - Scale(2.0f), minimum.y - Scale(2.0f)),
                     ImVec2(maximum.x + Scale(2.0f), maximum.y + Scale(2.0f)),
                     ImGui::GetColorU32(ToImVec4(CurrentPalette().focus)),
                     Scale(rounding + 2.0f), ImDrawFlags_RoundCornersAll,
                     Scale(2.0f));
}

bool DrawSliderFloat(const std::string_view id, float &value,
                     const float minimum, const float maximum,
                     const std::string_view format, const std::string_view unit,
                     const bool show_output, const bool framed,
                     const ImGuiSliderFlags flags) {
  const std::string owned_id = Owned(id);
  const std::string owned_format = Owned(format);
  const ImGuiID item_id = ImGui::GetID(owned_id.c_str());
  const float original_value = value;
  const bool editing_exact_value = ImGui::TempInputIsActive(item_id);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize,
                      CurrentLayoutMetrics().geometry.icon);
  if (!editing_exact_value) {
    PushInvisibleSliderStyle();
  }
  bool changed = ImGui::SliderFloat(owned_id.c_str(), &value, minimum, maximum,
                                    owned_format.c_str(), flags);
  if (!editing_exact_value) {
    PopInvisibleSliderStyle();
    const std::string output =
        show_output ? FormatSliderValue(original_value, ImGuiDataType_Float,
                                        format, unit)
                    : std::string{};
    changed = RetargetMouseSlider(
        item_id, value, original_value, ImGuiDataType_Float, minimum, maximum,
        owned_format.c_str(), flags, ResolveSliderGeometry(output, framed));
    DrawSliderPresentation(
        maximum > minimum ? (value - minimum) / (maximum - minimum) : 0.0f,
        show_output
            ? FormatSliderValue(value, ImGuiDataType_Float, format, unit)
            : std::string{},
        framed);
  }
  ImGui::PopStyleVar();
  return changed;
}

bool DrawSliderInt(const std::string_view id, int &value, const int minimum,
                   const int maximum, const bool show_output, const bool framed,
                   const ImGuiSliderFlags flags) {
  const std::string owned_id = Owned(id);
  const ImGuiID item_id = ImGui::GetID(owned_id.c_str());
  const int original_value = value;
  const bool editing_exact_value = ImGui::TempInputIsActive(item_id);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize,
                      CurrentLayoutMetrics().geometry.icon);
  if (!editing_exact_value) {
    PushInvisibleSliderStyle();
  }
  bool changed =
      ImGui::SliderInt(owned_id.c_str(), &value, minimum, maximum, "%d", flags);
  if (!editing_exact_value) {
    PopInvisibleSliderStyle();
    const std::string output =
        show_output
            ? FormatSliderValue(original_value, ImGuiDataType_S32, "%d", {})
            : std::string{};
    changed = RetargetMouseSlider(item_id, value, original_value,
                                  ImGuiDataType_S32, minimum, maximum, "%d",
                                  flags, ResolveSliderGeometry(output, framed));
    DrawSliderPresentation(
        maximum > minimum ? static_cast<float>(value - minimum) /
                                static_cast<float>(maximum - minimum)
                          : 0.0f,
        show_output ? FormatSliderValue(value, ImGuiDataType_S32, "%d", {})
                    : std::string{},
        framed);
  }
  ImGui::PopStyleVar();
  return changed;
}

void DrawValidationHint(const Validation &validation) {
  if (!validation.invalid || validation.message.empty()) {
    return;
  }
  ImGui::PushStyleColor(ImGuiCol_Text, StatusColor(SemanticStatus::Failure));
  ImGui::TextWrapped("%s", Owned(validation.message).c_str());
  ImGui::PopStyleColor();
}

ImVec4 StatusColor(const SemanticStatus status) {
  const SemanticPalette &palette = CurrentPalette();
  switch (status) {
  case SemanticStatus::Information:
  case SemanticStatus::Busy:
  case SemanticStatus::Preview:
    return ToImVec4(palette.information);
  case SemanticStatus::Success:
    return ToImVec4(palette.success);
  case SemanticStatus::Warning:
    return ToImVec4(palette.warning);
  case SemanticStatus::Failure:
    return ToImVec4(palette.failure);
  case SemanticStatus::Neutral:
    return ToImVec4(palette.text_secondary);
  }
  return ToImVec4(palette.text_secondary);
}

ImVec4 StatusBackground(const SemanticStatus status) {
  const SemanticPalette &palette = CurrentPalette();
  switch (status) {
  case SemanticStatus::Information:
  case SemanticStatus::Busy:
  case SemanticStatus::Preview:
    return ToImVec4(palette.information_background);
  case SemanticStatus::Success:
    return ToImVec4(palette.success_background);
  case SemanticStatus::Warning:
    return ToImVec4(palette.warning_background);
  case SemanticStatus::Failure:
    return ToImVec4(palette.failure_background);
  case SemanticStatus::Neutral:
    return ToImVec4(palette.surface_raised);
  }
  return ToImVec4(palette.surface_raised);
}

} // namespace fancy_ui::detail
