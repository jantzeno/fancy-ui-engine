#include "fancy_ui/components/hierarchy_row.hpp"

#include "fancy_ui/components/checkbox.hpp"
#include "fancy_ui/components/disclosure_row.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <optional>
#include <string>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
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
  return {.activated = activated && availability.enabled && !availability.busy,
          .interaction = interaction,
          .minimum = minimum,
          .maximum = maximum};
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

HierarchyRowResult HierarchyRow(HierarchyTree &tree,
                                const HierarchyRowSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const bool section_root = tree.open_nodes_ == 0;
  const float color_width =
      spec.color.has_value() ? metrics.geometry.compact_target : 0.0f;
  const float action_width =
      spec.action_icon ? metrics.geometry.compact_target : 0.0f;
  const float visibility_width =
      spec.visibility.has_value()
          ? (spec.visibility == ToggleState::Mixed
                 ? metrics.geometry.icon * 2.0f + Scale(2.0f)
                 : metrics.geometry.compact_target)
          : 0.0f;
  const float trailing_width = color_width + action_width + visibility_width;
  const DisclosureRowResult row = DisclosureRow({
      .id = spec.id,
      .label = spec.label,
      .metadata = spec.metadata,
      .tooltip = spec.tooltip,
      .variant = section_root ? DisclosureRowVariant::PanelHeader
                              : DisclosureRowVariant::Item,
      .expandable = spec.expandable,
      .expanded = spec.expanded,
      .selected = spec.selected,
      .font = section_root ? tree.section_font_ : FontHandle{},
      .leading_icon = spec.leading_icon,
      .status = spec.status,
      .reserved_trailing_width = trailing_width / CurrentUiScale(),
      .availability = spec.availability,
  });
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  if (row.expanded) {
    ImGui::TreePush(id.c_str());
    ++tree.open_nodes_;
  }
  const ImVec2 cursor_after = ImGui::GetCursorScreenPos();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const float center_y = (minimum.y + maximum.y) * 0.5f;

  std::optional<detail::ScopedInteractionPreview> inline_preview;
  if (detail::CurrentInteractionPreview().has_value()) {
    inline_preview.emplace(detail::InteractionPreview::Rest);
  }
  ImGui::PushID(id.c_str());
  float trailing_x = maximum.x - trailing_width;
  bool color_activated = false;
  if (spec.color.has_value()) {
    const InlineTargetResult color_target =
        InlineTarget("##color", ImVec2(trailing_x, center_y - Scale(12.0f)),
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

  bool visibility_changed = false;
  ToggleState visibility = spec.visibility.value_or(ToggleState::Off);
  if (spec.visibility.has_value()) {
    const std::string visibility_tooltip =
        !spec.visibility_tooltip.empty()
            ? detail::Owned(spec.visibility_tooltip)
        : visibility == ToggleState::On    ? "Hide"
        : visibility == ToggleState::Mixed ? "Show all descendants"
                                           : "Show";
    ImGui::SetCursorScreenPos(
        ImVec2(trailing_x, center_y - metrics.geometry.compact_target * 0.5f));
    const CheckboxResult visibility_target = Checkbox({
        .id = "visibility",
        .tooltip = visibility_tooltip,
        .state = visibility,
        .on_icon = spec.visible_icon,
        .off_icon = spec.hidden_icon,
        .show_checkbox = false,
        .availability = spec.availability,
    });
    visibility_changed = visibility_target.changed;
    if (visibility_changed) {
      visibility = visibility_target.state;
    }
    trailing_x += visibility_width;
  }

  bool action_activated = false;
  if (spec.action_icon) {
    action_activated =
        IconTarget("##action", ImVec2(trailing_x, center_y - Scale(12.0f)),
                   spec.action_icon,
                   disabled ? palette.text_disabled : palette.text_secondary,
                   spec.action_tooltip, spec.availability)
            .activated;
  }

  ImGui::PopID();
  if (trailing_width > 0.0f) {
    ImGui::SetCursorScreenPos(ImVec2(
        cursor_after.x, cursor_after.y - ImGui::GetStyle().ItemSpacing.y));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
  }

  HierarchyRowResult result;
  static_cast<InteractionResult &>(result) = row;
  result.activated = row.activated && !color_activated && !action_activated &&
                     !visibility_changed;
  result.additive = result.activated && ImGui::GetIO().KeyCtrl;
  result.range = result.activated && ImGui::GetIO().KeyShift;
  result.expansion_changed = row.expansion_changed;
  result.expanded = row.expanded;
  result.color_activated = color_activated && !disabled;
  result.action_activated = action_activated && !disabled;
  result.visibility_changed = visibility_changed && !disabled;
  result.visibility = visibility;
  return result;
}

} // namespace fancy_ui
