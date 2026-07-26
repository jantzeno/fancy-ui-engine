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
  const SemanticPalette &palette = CurrentPalette();

  const auto to_imgui = [](const ColorRgba color) {
    return ImVec4(color.red, color.green, color.blue, color.alpha);
  };
  ImVec4 rest = to_imgui(palette.control);
  ImVec4 hover = to_imgui(palette.control_hover);
  ImVec4 pressed = to_imgui(palette.control_pressed);
  ImVec4 text = to_imgui(palette.text_primary);
  if (spec.variant == ButtonVariant::Primary) {
    rest = to_imgui(palette.action_primary);
    hover = to_imgui(palette.action_primary_hover);
    pressed = to_imgui(palette.action_primary_pressed);
    text = to_imgui(palette.on_emphasis);
  } else if (spec.variant == ButtonVariant::Tertiary) {
    rest.w = 0.0f;
  } else if (spec.variant == ButtonVariant::Destructive) {
    text = to_imgui(palette.failure);
  }

  ImGui::PushID(id.c_str());
  ImGui::PushStyleColor(ImGuiCol_Button, rest);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, pressed);
  ImGui::PushStyleColor(ImGuiCol_Text, text);
  detail::BeginAvailability(spec.availability);
  const bool activated =
      ImGui::Button(label.c_str(), ImVec2(spec.size.x, spec.size.y));
  const InteractionResult interaction = detail::CaptureInteraction();
  detail::EndAvailability(spec.availability, spec.tooltip);
  ImGui::PopStyleColor(4);
  ImGui::PopID();

  ButtonResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = activated;
  return result;
}

} // namespace fancy_ui
