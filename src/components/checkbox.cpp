#include "fancy_ui/components/checkbox.hpp"

#include "fancy_ui/layout_metrics.hpp"
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
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ImGui::PushFont(nullptr, metrics.typography.body_font_height);
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const SemanticPalette &palette = CurrentPalette();
  const float box_size = Scale(16.0f);
  const float icon_size = Scale(16.0f);
  const float gap = Scale(8.0f);
  const float target_height = metrics.geometry.compact_target;
  const ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
  const bool has_state_icon =
      static_cast<bool>(spec.on_icon) || static_cast<bool>(spec.off_icon);
  const bool dual_state_icons = spec.state == ToggleState::Mixed &&
                                static_cast<bool>(spec.on_icon) &&
                                static_cast<bool>(spec.off_icon);
  const float icon_gap = Scale(2.0f);
  const float icon_width = has_state_icon
                               ? icon_size * (dual_state_icons ? 2.0f : 1.0f) +
                                     (dual_state_icons ? icon_gap : 0.0f)
                               : 0.0f;
  const float content_width =
      (spec.show_checkbox
           ? box_size + ((has_state_icon || !label.empty()) ? gap : 0.0f)
           : 0.0f) +
      icon_width + ((has_state_icon && !label.empty()) ? gap : 0.0f) +
      text_size.x;
  const float target_width =
      std::min(std::max(content_width, Scale(24.0f)),
               std::max(ImGui::GetContentRegionAvail().x, Scale(1.0f)));

  ImGui::PushID(id.c_str());
  detail::BeginAvailability(spec.availability);
  const bool activated = ImGui::InvisibleButton(
      "##checkbox", ImVec2(target_width, std::max(target_height, text_size.y)),
      ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const float center_y = (minimum.y + maximum.y) * 0.5f;
  float cursor_x = minimum.x;
  const ColorRgba foreground =
      disabled ? palette.text_disabled : palette.text_primary;
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->PushClipRect(minimum, maximum, true);
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
    const ColorRgba icon_color =
        disabled ? palette.text_disabled : palette.focus;
    const auto draw_icon = [&](const IconPainter &painter) {
      if (painter) {
        painter({.minimum = {.x = cursor_x, .y = center_y - icon_size * 0.5f},
                 .maximum = {.x = cursor_x + icon_size,
                             .y = center_y + icon_size * 0.5f}},
                icon_color);
      }
      cursor_x += icon_size;
    };
    if (dual_state_icons) {
      draw_icon(spec.on_icon);
      cursor_x += icon_gap;
      draw_icon(spec.off_icon);
    } else {
      draw_icon(spec.state == ToggleState::On ? spec.on_icon : spec.off_icon);
    }
    if (!label.empty()) {
      cursor_x += gap;
    }
  }
  const std::string visible_label =
      detail::EllipsizeText(label, std::max(0.0f, maximum.x - cursor_x));
  draw_list->AddText(
      ImVec2(cursor_x, std::floor(center_y - text_size.y * 0.5f)),
      ImGui::GetColorU32(ToImVec4(foreground)), visible_label.c_str());
  draw_list->PopClipRect();
  detail::DrawFocusRing(interaction);
  const std::string_view tooltip =
      spec.tooltip.empty() && visible_label != label ? std::string_view(label)
                                                     : spec.tooltip;
  detail::EndAvailability(spec.availability, tooltip);
  ImGui::PopID();
  detail::DrawValidationHint(spec.validation);
  ImGui::PopFont();

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
