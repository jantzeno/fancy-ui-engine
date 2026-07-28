#include "fancy_ui/components/button.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <string>

namespace fancy_ui {

ButtonResult Button(const ButtonSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string label =
      detail::Owned(spec.availability.busy ? std::string(spec.label) + "..."
                                           : std::string(spec.label));
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const auto colors_for = [&spec, disabled](const bool hovered,
                                            const bool pressed,
                                            const bool focused = false) {
    return detail::ResolveControlColors({
        .disabled = disabled,
        .selected = spec.selected,
        .invalid = spec.validation.invalid,
        .hovered = hovered,
        .pressed = pressed,
        .focused = focused,
        .primary = spec.variant == ButtonVariant::Primary,
        .tertiary = spec.variant == ButtonVariant::Tertiary,
        .destructive = spec.variant == ButtonVariant::Destructive,
    });
  };
  detail::ControlColors rest = colors_for(false, false);
  detail::ControlColors hover = colors_for(true, false);
  detail::ControlColors pressed = colors_for(true, true);
  if (const auto preview = detail::CurrentInteractionPreview();
      preview.has_value()) {
    const bool preview_hovered =
        *preview == detail::InteractionPreview::Hovered ||
        *preview == detail::InteractionPreview::Pressed;
    const bool preview_pressed =
        *preview == detail::InteractionPreview::Pressed;
    rest = hover = pressed =
        colors_for(preview_hovered, preview_pressed,
                   *preview == detail::InteractionPreview::Focused);
  }
  const auto to_imgui = [](const ColorRgba color) {
    return ImVec4(color.red, color.green, color.blue, color.alpha);
  };
  const auto scaled_extent = [](const float value) {
    return value <= 0.0f ? value : Scale(value);
  };

  ImGui::PushID(id.c_str());
  ImGui::PushStyleColor(ImGuiCol_Button, to_imgui(rest.fill));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_imgui(hover.fill));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, to_imgui(pressed.fill));
  ImGui::PushStyleColor(ImGuiCol_Text, to_imgui(rest.text));
  ImGui::PushStyleColor(ImGuiCol_Border, to_imgui(rest.border));
  detail::BeginAvailability(spec.availability);
  const bool activated =
      ImGui::Button(label.c_str(), ImVec2(scaled_extent(spec.size.x),
                                          scaled_extent(spec.size.y)));
  const InteractionResult interaction = detail::CaptureInteraction();
  detail::DrawFocusRing(interaction,
                        spec.variant == ButtonVariant::Primary ||
                            spec.variant == ButtonVariant::Destructive);
  detail::EndAvailability(spec.availability, spec.tooltip);
  ImGui::PopStyleColor(5);
  ImGui::PopID();
  detail::DrawValidationHint(spec.validation);

  ButtonResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = activated && !disabled;
  return result;
}

} // namespace fancy_ui
