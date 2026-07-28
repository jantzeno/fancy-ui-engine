#include "fancy_ui/components/hierarchy.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

ColorRgba StatusForeground(const SemanticStatus status) {
  const SemanticPalette &palette = CurrentPalette();
  switch (status) {
  case SemanticStatus::Information:
  case SemanticStatus::Busy:
  case SemanticStatus::Preview:
    return palette.information;
  case SemanticStatus::Success:
    return palette.success;
  case SemanticStatus::Warning:
    return palette.warning;
  case SemanticStatus::Failure:
    return palette.failure;
  case SemanticStatus::Neutral:
    return palette.text_primary;
  }
  return palette.text_primary;
}

struct InlineTargetResult {
  bool activated = false;
  InteractionResult interaction;
  ImVec2 minimum;
  ImVec2 maximum;
};

InlineTargetResult InlineTarget(const char *id, const ImVec2 position,
                                const Availability &availability,
                                const std::string_view tooltip) {
  ImGui::SetCursorScreenPos(position);
  ImGui::SetNextItemAllowOverlap();
  detail::BeginAvailability(availability);
  const bool activated = ImGui::InvisibleButton(
      id, ImVec2(Scale(24.0f), Scale(24.0f)), ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  if (interaction.hovered && availability.enabled && !availability.busy) {
    ImGui::GetWindowDrawList()->AddRectFilled(
        minimum, maximum,
        ImGui::GetColorU32(ToImVec4(CurrentPalette().control_hover)),
        Scale(3.0f));
  }
  detail::DrawFocusRing(interaction);
  detail::EndAvailability(availability, tooltip);
  return {
      .activated = activated && availability.enabled && !availability.busy,
      .interaction = interaction,
      .minimum = minimum,
      .maximum = maximum,
  };
}

InlineTargetResult IconTarget(const char *id, const ImVec2 position,
                              const IconPainter &painter, const ColorRgba color,
                              const std::string_view tooltip,
                              const Availability &availability) {
  const InlineTargetResult target =
      InlineTarget(id, position, availability, tooltip);
  if (painter) {
    const float inset = Scale(4.0f);
    painter({.minimum = {.x = target.minimum.x + inset,
                         .y = target.minimum.y + inset},
             .maximum = {.x = target.maximum.x - inset,
                         .y = target.maximum.y - inset}},
            color);
  }
  return target;
}

} // namespace

ToggleState
AggregateVisibility(const std::span<const ToggleState> descendants) {
  if (descendants.empty()) {
    return ToggleState::Off;
  }
  const ToggleState first = descendants.front();
  if (first == ToggleState::Mixed) {
    return ToggleState::Mixed;
  }
  for (const ToggleState state : descendants.subspan(1)) {
    if (state == ToggleState::Mixed || state != first) {
      return ToggleState::Mixed;
    }
  }
  return first;
}

ToggleState NextVisibilityState(const ToggleState current) {
  return current == ToggleState::On ? ToggleState::Off : ToggleState::On;
}

HierarchyRowResult HierarchyRow(const HierarchyRowSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const SemanticPalette &palette = CurrentPalette();
  const float height = Scale(32.0f);
  const float available_width =
      std::max(ImGui::GetContentRegionAvail().x, Scale(1.0f));
  const float color_width = spec.color.has_value() ? Scale(32.0f) : 0.0f;
  const float action_width = spec.action_icon ? Scale(28.0f) : 0.0f;
  const float visibility_width =
      spec.visibility.has_value() ? Scale(28.0f) : 0.0f;
  const float trailing_width = color_width + action_width + visibility_width;

  ImGui::PushID(id.c_str());
  detail::BeginAvailability(spec.availability);
  ImGui::SetNextItemAllowOverlap();
  const bool row_clicked = ImGui::InvisibleButton(
      "##row", ImVec2(available_width, height), ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const ImVec2 cursor_after = ImGui::GetCursorScreenPos();
  detail::EndAvailability(spec.availability, spec.tooltip);

  detail::ControlColors colors = detail::ResolveControlColors({
      .disabled = disabled,
      .selected = spec.selected,
      .invalid = spec.status == SemanticStatus::Failure,
      .hovered = interaction.hovered,
      .pressed = interaction.active,
      .focused = interaction.focused,
  });
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (spec.selected || interaction.hovered || interaction.active) {
    draw_list->AddRectFilled(minimum, maximum,
                             ImGui::GetColorU32(ToImVec4(colors.fill)));
  }
  if (spec.selected) {
    draw_list->AddRectFilled(minimum,
                             ImVec2(minimum.x + Scale(3.0f), maximum.y),
                             ImGui::GetColorU32(ToImVec4(palette.focus)));
  }
  if (spec.status == SemanticStatus::Failure) {
    draw_list->AddRect(minimum, maximum,
                       ImGui::GetColorU32(ToImVec4(palette.failure)), 0.0f, 0,
                       Scale(1.0f));
  }
  detail::DrawFocusRing(interaction);

  float text_x = minimum.x + Scale(8.0f) +
                 Scale(12.0f) * static_cast<float>(std::max(spec.depth, 0));
  const float center_y = (minimum.y + maximum.y) * 0.5f;
  bool expansion_changed = false;
  bool expanded = spec.expanded;
  if (spec.expandable) {
    const ImVec2 expander_position(text_x, center_y - Scale(12.0f));
    const std::string expansion_tooltip =
        std::string(spec.expanded ? "Collapse " : "Expand ") +
        detail::Owned(spec.label);
    const InlineTargetResult expander = InlineTarget(
        "##expander", expander_position, spec.availability, expansion_tooltip);
    if (expander.activated) {
      expansion_changed = true;
      expanded = !expanded;
    }
    const ImVec2 center(expander_position.x + Scale(12.0f),
                        expander_position.y + Scale(12.0f));
    const ImU32 triangle_color = ImGui::GetColorU32(
        ToImVec4(disabled ? palette.text_disabled : palette.text_secondary));
    if (expanded) {
      draw_list->AddTriangleFilled(
          ImVec2(center.x - Scale(5.0f), center.y - Scale(2.0f)),
          ImVec2(center.x + Scale(5.0f), center.y - Scale(2.0f)),
          ImVec2(center.x, center.y + Scale(4.0f)), triangle_color);
    } else {
      draw_list->AddTriangleFilled(
          ImVec2(center.x - Scale(2.0f), center.y - Scale(5.0f)),
          ImVec2(center.x - Scale(2.0f), center.y + Scale(5.0f)),
          ImVec2(center.x + Scale(4.0f), center.y), triangle_color);
    }
    text_x += Scale(24.0f);
  } else {
    text_x += Scale(16.0f);
  }

  if (spec.leading_icon) {
    const float icon_size = Scale(16.0f);
    spec.leading_icon(
        {
            .minimum = {.x = text_x, .y = center_y - icon_size * 0.5f},
            .maximum = {.x = text_x + icon_size,
                        .y = center_y + icon_size * 0.5f},
        },
        disabled ? palette.text_disabled : StatusForeground(spec.status));
    text_x += Scale(20.0f);
  }

  const ColorRgba label_color = disabled ? palette.text_disabled
                                : spec.status == SemanticStatus::Neutral
                                    ? colors.text
                                    : StatusForeground(spec.status);
  const ImVec2 label_size =
      ImGui::CalcTextSize(detail::Owned(spec.label).c_str());
  const float text_y = spec.secondary_label.empty()
                           ? std::floor(center_y - label_size.y * 0.5f)
                           : minimum.y + Scale(3.0f);
  draw_list->PushClipRect(
      ImVec2(text_x, minimum.y),
      ImVec2(maximum.x - trailing_width - Scale(4.0f), maximum.y), true);
  draw_list->AddText(ImVec2(text_x, text_y),
                     ImGui::GetColorU32(ToImVec4(label_color)),
                     detail::Owned(spec.label).c_str());
  if (!spec.secondary_label.empty()) {
    draw_list->AddText(
        ImVec2(text_x, minimum.y + Scale(17.0f)),
        ImGui::GetColorU32(ToImVec4(disabled ? palette.text_disabled
                                             : palette.text_secondary)),
        detail::Owned(spec.secondary_label).c_str());
  }
  draw_list->PopClipRect();

  float trailing_x = maximum.x - trailing_width;
  bool color_activated = false;
  if (spec.color.has_value()) {
    const InlineTargetResult color_target = InlineTarget(
        "##color", ImVec2(trailing_x + Scale(4.0f), center_y - Scale(12.0f)),
        spec.availability, spec.color_tooltip);
    color_activated = color_target.activated;
    const ImVec2 swatch_min(color_target.minimum.x + Scale(5.0f),
                            color_target.minimum.y + Scale(5.0f));
    const ImVec2 swatch_max(swatch_min.x + Scale(14.0f),
                            swatch_min.y + Scale(14.0f));
    draw_list->AddRectFilled(swatch_min, swatch_max,
                             ImGui::GetColorU32(ToImVec4(*spec.color)),
                             Scale(2.0f));
    draw_list->AddRect(swatch_min, swatch_max,
                       ImGui::GetColorU32(ToImVec4(palette.border_strong)),
                       Scale(2.0f));
    if (spec.request_color_focus) {
      ImGui::SetKeyboardFocusHere(-1);
    }
    trailing_x += color_width;
  }

  bool action_activated = false;
  if (spec.action_icon) {
    action_activated =
        IconTarget("##action",
                   ImVec2(trailing_x + Scale(2.0f), center_y - Scale(12.0f)),
                   spec.action_icon,
                   disabled ? palette.text_disabled : palette.text_secondary,
                   spec.action_tooltip, spec.availability)
            .activated;
    trailing_x += action_width;
  }

  bool visibility_changed = false;
  ToggleState visibility = spec.visibility.value_or(ToggleState::Off);
  if (spec.visibility.has_value()) {
    const IconPainter &icon =
        visibility == ToggleState::Off ? spec.hidden_icon : spec.visible_icon;
    const std::string visibility_tooltip =
        !spec.visibility_tooltip.empty()
            ? detail::Owned(spec.visibility_tooltip)
        : visibility == ToggleState::On    ? "Hide"
        : visibility == ToggleState::Mixed ? "Show all descendants"
                                           : "Show";
    const InlineTargetResult visibility_target = IconTarget(
        "##visibility",
        ImVec2(trailing_x + Scale(2.0f), center_y - Scale(12.0f)), icon,
        disabled ? palette.text_disabled : palette.text_secondary,
        visibility_tooltip, spec.availability);
    visibility_changed = visibility_target.activated;
    if (visibility == ToggleState::Mixed) {
      draw_list->AddLine(
          ImVec2(visibility_target.minimum.x + Scale(7.0f),
                 visibility_target.maximum.y - Scale(4.0f)),
          ImVec2(visibility_target.maximum.x - Scale(7.0f),
                 visibility_target.maximum.y - Scale(4.0f)),
          ImGui::GetColorU32(ToImVec4(disabled ? palette.text_disabled
                                               : palette.text_primary)),
          Scale(2.0f));
    }
    if (visibility_changed) {
      visibility = NextVisibilityState(visibility);
    }
  }

  bool keyboard_expansion = false;
  if (interaction.focused) {
    if (spec.expandable && spec.expanded &&
        ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
      expanded = false;
      keyboard_expansion = true;
    } else if (spec.expandable && !spec.expanded &&
               ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
      expanded = true;
      keyboard_expansion = true;
    }
  }
  ImGui::SetCursorScreenPos(cursor_after);
  ImGui::Dummy(ImVec2(0.0f, 0.0f));
  ImGui::PopID();

  HierarchyRowResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = row_clicked && !disabled && !expansion_changed &&
                     !color_activated && !action_activated &&
                     !visibility_changed;
  result.additive = result.activated && ImGui::GetIO().KeyCtrl;
  result.range = result.activated && ImGui::GetIO().KeyShift;
  result.expansion_changed = expansion_changed || keyboard_expansion;
  result.expanded = expanded;
  result.color_activated = color_activated && !disabled;
  result.action_activated = action_activated && !disabled;
  result.visibility_changed = visibility_changed && !disabled;
  result.visibility = visibility;
  return result;
}

} // namespace fancy_ui
