#include "internal/component_internal.hpp"

#include "fancy_ui/theme.hpp"

namespace fancy_ui::detail {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

std::string Owned(const std::string_view value) {
  return std::string(value.data(), value.size());
}

void BeginAvailability(const Availability &availability) {
  ImGui::BeginDisabled(!availability.enabled || availability.busy);
}

void EndAvailability(const Availability &availability,
                     const std::string_view tooltip) {
  const bool hovered =
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
  ImGui::EndDisabled();
  if (!hovered) {
    return;
  }

  if (!availability.enabled && !availability.reason.empty()) {
    const std::string reason = Owned(availability.reason);
    ImGui::SetTooltip("%s", reason.c_str());
  } else if (!tooltip.empty()) {
    const std::string text = Owned(tooltip);
    ImGui::SetTooltip("%s", text.c_str());
  }
}

InteractionResult CaptureInteraction() {
  return {
      .hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
      .focused = ImGui::IsItemFocused(),
      .active = ImGui::IsItemActive(),
  };
}

ImVec4 StatusColor(const SemanticStatus status) {
  const SemanticPalette &palette = CurrentPalette();
  switch (status) {
  case SemanticStatus::Information:
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
