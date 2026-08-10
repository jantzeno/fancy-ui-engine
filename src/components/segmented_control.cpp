#include "fancy_ui/components/segmented_control.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

SegmentedControlResult SegmentedControl(const SegmentedControlSpec &spec) {
  SegmentedControlResult result;
  if (spec.choices.empty()) {
    return result;
  }

  struct Segment {
    const ChoiceSpec *choice = nullptr;
    ImVec2 minimum;
    ImVec2 maximum;
    InteractionResult interaction;
    bool disabled = false;
  };
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const std::size_t selected =
      std::min(spec.selected_index, spec.choices.size() - std::size_t{1});
  const float equal_width =
      spec.width > 0.0f
          ? Scale(spec.width) / static_cast<float>(spec.choices.size())
          : 0.0f;
  std::vector<Segment> segments;
  segments.reserve(spec.choices.size());

  ImGui::PushFont(nullptr, metrics.typography.body_font_height);
  ImGui::PushID(detail::Owned(spec.id).c_str());
  for (std::size_t index = 0; index < spec.choices.size(); ++index) {
    if (index > 0) {
      ImGui::SameLine(0.0f, 0.0f);
    }
    const ChoiceSpec &choice = spec.choices[index];
    const bool disabled =
        !choice.availability.enabled || choice.availability.busy;
    const float width =
        equal_width > 0.0f
            ? equal_width
            : std::max(
                  Scale(56.0f),
                  ImGui::CalcTextSize(detail::Owned(choice.label).c_str()).x +
                      metrics.spacing.space06);
    ImGui::PushID(detail::Owned(choice.id).c_str());
    detail::BeginAvailability(choice.availability);
    const bool activated = ImGui::InvisibleButton(
        "##segment", ImVec2(width, metrics.geometry.control_height),
        ImGuiButtonFlags_EnableNav);
    const InteractionResult interaction = detail::CaptureInteraction();
    result.hovered = result.hovered || interaction.hovered;
    result.focused = result.focused || interaction.focused;
    result.active = result.active || interaction.active;
    detail::EndAvailability(choice.availability, choice.tooltip);
    segments.push_back({.choice = &choice,
                        .minimum = ImGui::GetItemRectMin(),
                        .maximum = ImGui::GetItemRectMax(),
                        .interaction = interaction,
                        .disabled = disabled});
    if (activated && !disabled && index != selected) {
      result.changed = true;
      result.selected_index = index;
    }
    if (interaction.focused) {
      const int direction = ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) ? -1
                            : ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)
                                ? 1
                                : 0;
      if (direction != 0) {
        std::size_t candidate = index;
        for (std::size_t tries = 0; tries < spec.choices.size(); ++tries) {
          candidate = static_cast<std::size_t>(
              (static_cast<int>(candidate) + direction +
               static_cast<int>(spec.choices.size())) %
              static_cast<int>(spec.choices.size()));
          const Availability &availability =
              spec.choices[candidate].availability;
          if (availability.enabled && !availability.busy) {
            result.changed = candidate != selected;
            result.selected_index = candidate;
            break;
          }
        }
      }
    }
    ImGui::PopID();
  }

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const auto corners = [&segments](const std::size_t index) {
    if (segments.size() == 1) {
      return ImDrawFlags_RoundCornersAll;
    }
    if (index == 0) {
      return ImDrawFlags_RoundCornersLeft;
    }
    return index + 1 == segments.size() ? ImDrawFlags_RoundCornersRight
                                        : ImDrawFlags_RoundCornersNone;
  };
  for (std::size_t index = 0; index < segments.size(); ++index) {
    const Segment &segment = segments[index];
    const bool is_selected = index == selected;
    detail::ControlColors colors = detail::ResolveControlColors({
        .disabled = segment.disabled,
        .selected = is_selected,
        .hovered = segment.interaction.hovered,
        .pressed = segment.interaction.active,
    });
    if (!is_selected && !segment.disabled && !segment.interaction.hovered &&
        !segment.interaction.active) {
      colors.fill = palette.surface_raised;
    }
    draw_list->AddRectFilled(segment.minimum, segment.maximum,
                             ImGui::GetColorU32(ToImVec4(colors.fill)),
                             metrics.geometry.control_radius, corners(index));
    draw_list->AddRect(segment.minimum, segment.maximum,
                       ImGui::GetColorU32(ToImVec4(colors.border)),
                       metrics.geometry.control_radius, corners(index),
                       metrics.geometry.border);
    const std::string label = detail::Owned(segment.choice->label);
    const ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
    draw_list->AddText(
        ImVec2(
            std::floor((segment.minimum.x + segment.maximum.x - label_size.x) *
                       0.5f),
            std::floor((segment.minimum.y + segment.maximum.y - label_size.y) *
                       0.5f)),
        ImGui::GetColorU32(ToImVec4(colors.text)), label.c_str());
    if (is_selected) {
      draw_list->AddRectFilled(
          ImVec2(segment.minimum.x + metrics.geometry.border,
                 segment.maximum.y - Scale(3.0f)),
          ImVec2(segment.maximum.x - metrics.geometry.border,
                 segment.maximum.y - metrics.geometry.border),
          ImGui::GetColorU32(ToImVec4(palette.focus)));
    }
    if (segment.interaction.focused && ImGui::GetIO().NavVisible) {
      draw_list->AddRect(ImVec2(segment.minimum.x + Scale(3.0f),
                                segment.minimum.y + Scale(3.0f)),
                         ImVec2(segment.maximum.x - Scale(3.0f),
                                segment.maximum.y - Scale(3.0f)),
                         ImGui::GetColorU32(ToImVec4(palette.focus)),
                         metrics.geometry.control_radius, corners(index),
                         metrics.geometry.focus_ring);
    }
  }
  ImGui::PopID();
  ImGui::PopFont();
  if (!result.changed) {
    result.selected_index = selected;
  }
  return result;
}

} // namespace fancy_ui
