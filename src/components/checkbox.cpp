#include "fancy_ui/components/checkbox.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

CheckboxResult Checkbox(const CheckboxSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const SemanticPalette &palette = CurrentPalette();
  const float box_size = Scale(16.0f);
  const float icon_size = Scale(16.0f);
  const float gap = Scale(8.0f);
  const float target_height = Scale(24.0f);
  const ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
  const bool has_state_icon =
      static_cast<bool>(spec.on_icon) || static_cast<bool>(spec.off_icon);
  const float content_width = (spec.show_checkbox ? box_size + gap : 0.0f) +
                              (has_state_icon ? icon_size + gap : 0.0f) +
                              text_size.x;

  ImGui::PushID(id.c_str());
  detail::BeginAvailability(spec.availability);
  const bool activated =
      ImGui::InvisibleButton("##checkbox",
                             ImVec2(std::max(content_width, Scale(24.0f)),
                                    std::max(target_height, text_size.y)),
                             ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const float center_y = (minimum.y + maximum.y) * 0.5f;
  float cursor_x = minimum.x;
  const ColorRgba foreground =
      disabled ? palette.text_disabled : palette.text_primary;
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (!disabled && (interaction.hovered || interaction.active)) {
    draw_list->AddRectFilled(minimum, maximum,
                             ImGui::GetColorU32(ToImVec4(
                                 interaction.active ? palette.control_pressed
                                                    : palette.control_hover)),
                             Scale(3.0f));
  }

  if (spec.show_checkbox) {
    const ImVec2 box_min(cursor_x, center_y - box_size * 0.5f);
    const ImVec2 box_max(cursor_x + box_size, center_y + box_size * 0.5f);
    const bool marked = spec.state != ToggleState::Off;
    draw_list->AddRectFilled(
        box_min, box_max,
        ImGui::GetColorU32(ToImVec4(disabled ? palette.control_disabled_fill
                                    : marked ? palette.action_primary
                                             : palette.surface_raised)),
        Scale(2.0f));
    draw_list->AddRect(
        box_min, box_max,
        ImGui::GetColorU32(ToImVec4(disabled ? palette.control_disabled_border
                                    : marked ? palette.action_primary
                                             : palette.border_strong)),
        Scale(2.0f), ImDrawFlags_RoundCornersAll, Scale(1.0f));
    const ImU32 mark_color = ImGui::GetColorU32(
        ToImVec4(disabled ? palette.text_disabled : palette.on_emphasis));
    if (spec.state == ToggleState::On) {
      draw_list->AddLine(
          ImVec2(box_min.x + box_size * 0.22f, box_min.y + box_size * 0.52f),
          ImVec2(box_min.x + box_size * 0.43f, box_min.y + box_size * 0.73f),
          mark_color, Scale(2.0f));
      draw_list->AddLine(
          ImVec2(box_min.x + box_size * 0.43f, box_min.y + box_size * 0.73f),
          ImVec2(box_min.x + box_size * 0.80f, box_min.y + box_size * 0.28f),
          mark_color, Scale(2.0f));
    } else if (spec.state == ToggleState::Mixed) {
      draw_list->AddLine(ImVec2(box_min.x + box_size * 0.24f, center_y),
                         ImVec2(box_max.x - box_size * 0.24f, center_y),
                         mark_color, Scale(2.0f));
    }
    cursor_x += box_size + gap;
  }

  if (has_state_icon) {
    const Rect bounds{
        .minimum = {.x = cursor_x, .y = center_y - icon_size * 0.5f},
        .maximum = {.x = cursor_x + icon_size,
                    .y = center_y + icon_size * 0.5f},
    };
    const IconPainter &painter =
        spec.state == ToggleState::On ? spec.on_icon : spec.off_icon;
    if (painter) {
      painter(bounds, disabled ? palette.text_disabled : palette.focus);
    }
    cursor_x += icon_size + gap;
  }
  draw_list->AddText(
      ImVec2(cursor_x, std::floor(center_y - text_size.y * 0.5f)),
      ImGui::GetColorU32(ToImVec4(foreground)), label.c_str());
  detail::DrawFocusRing(interaction);
  detail::EndAvailability(spec.availability, spec.tooltip);
  ImGui::PopID();
  detail::DrawValidationHint(spec.validation);

  CheckboxResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = activated && !disabled;
  result.state =
      result.changed
          ? (spec.state == ToggleState::On ? ToggleState::Off : ToggleState::On)
          : spec.state;
  return result;
}

} // namespace fancy_ui
