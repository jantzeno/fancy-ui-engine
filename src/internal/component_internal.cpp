#include "internal/component_internal.hpp"

#include "fancy_ui/theme.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace fancy_ui::detail {

namespace {

thread_local std::optional<InteractionPreview> interaction_preview;

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
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

std::string Owned(const std::string_view value) {
  return std::string(value.data(), value.size());
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
  const std::string owned_label = Owned(label);
  if (ImGui::GetContentRegionAvail().x < Scale(288.0f)) {
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_secondary));
    ImGui::TextUnformatted(owned_label.c_str());
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(-std::numeric_limits<float>::min());
    return {};
  }

  const bool table = ImGui::BeginTable("##field-layout", 2,
                                       ImGuiTableFlags_SizingStretchProp |
                                           ImGuiTableFlags_NoSavedSettings);
  if (table) {
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed,
                            Scale(112.0f));
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_secondary));
    ImGui::TextUnformatted(owned_label.c_str());
    ImGui::PopStyleColor();
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-std::numeric_limits<float>::min());
  }
  return {.table = table};
}

void EndFieldLayout(const FieldLayout layout, const Validation &validation) {
  if (layout.table) {
    ImGui::EndTable();
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
  }
}

void PopFieldControlState(const Availability &availability,
                          const Validation &validation) {
  if (!availability.enabled || availability.busy) {
    ImGui::PopStyleColor(3);
  } else if (validation.invalid) {
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
    const std::string reason = Owned(availability.reason);
    ImGui::SetTooltip("%s", reason.c_str());
  } else if (!tooltip.empty()) {
    const std::string text = Owned(tooltip);
    ImGui::SetTooltip("%s", text.c_str());
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
