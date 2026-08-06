#include "fancy_ui/components/hierarchy.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <limits>
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

void DrawTreeConnector(ImDrawList *draw_list, const int depth,
                       const ImVec2 node_cursor, const ImVec2 minimum,
                       const ImVec2 maximum, const float row_height,
                       const LayoutMetrics &metrics, const ColorRgba color) {
  if (depth <= 0) {
    return;
  }
  const float elbow_x =
      node_cursor.x - metrics.explorer.tree_indent + metrics.spacing.space02;
  const float center_y = (minimum.y + maximum.y) * 0.5f;
  const float start_y =
      minimum.y - row_height * 0.5f - metrics.spacing.condensed;
  const ImU32 native_color = ImGui::GetColorU32(ToImVec4(color));
  draw_list->AddLine(ImVec2(elbow_x, start_y), ImVec2(elbow_x, center_y),
                     native_color, metrics.explorer.tree_connector_thickness);
  draw_list->AddLine(ImVec2(elbow_x, center_y), ImVec2(node_cursor.x, center_y),
                     native_color, metrics.explorer.tree_connector_thickness);
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

HierarchyTree::HierarchyTree() {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const ImGuiStyle &style = ImGui::GetStyle();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(style.ItemSpacing.x, metrics.spacing.condensed));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing,
                      metrics.explorer.tree_indent);
}

HierarchyTree::~HierarchyTree() {
  const bool unbalanced = open_nodes_ != 0;
  while (open_nodes_ > 0) {
    ImGui::TreePop();
    --open_nodes_;
  }
  ImGui::PopStyleVar(2);
  IM_ASSERT(!unbalanced &&
            "Every expanded hierarchy row must have a matching Pop()");
}

void HierarchyTree::Pop() {
  IM_ASSERT(open_nodes_ > 0 && "Cannot pop a hierarchy with no open parent");
  if (open_nodes_ <= 0) {
    return;
  }
  ImGui::TreePop();
  --open_nodes_;
}

HierarchyRowResult HierarchyRow(HierarchyTree &tree,
                                const HierarchyRowSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  const std::string secondary_label = detail::Owned(spec.secondary_label);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const float row_height = secondary_label.empty()
                               ? metrics.geometry.row_height
                               : metrics.explorer.detailed_tree_row_height;
  const float vertical_padding =
      std::max((row_height - ImGui::GetFontSize()) * 0.5f, 0.0f);
  const int depth = tree.open_nodes_;

  const ImVec2 node_cursor = ImGui::GetCursorScreenPos();
  const std::string native_id = "##" + id;
  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow |
      ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding |
      ImGuiTreeNodeFlags_NavLeftJumpsToParent;
  if (spec.selected) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  if (!spec.expandable) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  } else {
    ImGui::SetNextItemOpen(spec.expanded, ImGuiCond_Always);
  }

  detail::BeginAvailability(spec.availability);
  ImGui::SetNextItemAllowOverlap();
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(metrics.spacing.space03, vertical_padding));
  ImGui::PushStyleColor(
      ImGuiCol_Text,
      ToImVec4(disabled ? palette.text_disabled : palette.text_secondary));
  const bool native_open = ImGui::TreeNodeEx(native_id.c_str(), flags);
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const ImVec2 cursor_after = ImGui::GetCursorScreenPos();
  const bool labeled_columns =
      maximum.x - minimum.x >= metrics.explorer.labeled_audit_breakpoint;
  const float color_width =
      spec.color.has_value()
          ? labeled_columns ? metrics.explorer.labeled_audit_color_column_width
                            : metrics.explorer.audit_color_column_width
          : 0.0f;
  const float action_width =
      spec.action_icon
          ? labeled_columns ? metrics.explorer.labeled_audit_action_column_width
                            : metrics.explorer.audit_action_column_width
          : 0.0f;
  const float visibility_width =
      spec.visibility.has_value()
          ? labeled_columns
                ? metrics.explorer.labeled_audit_visibility_column_width
                : metrics.explorer.audit_visibility_column_width
          : 0.0f;
  const float trailing_width = color_width + action_width + visibility_width;
  const bool expansion_changed = spec.expandable && ImGui::IsItemToggledOpen();
  const bool pointer_over_trailing =
      trailing_width > 0.0f &&
      ImGui::GetIO().MousePos.x >= maximum.x - trailing_width &&
      ImGui::GetIO().MousePos.x < maximum.x &&
      ImGui::GetIO().MousePos.y >= minimum.y &&
      ImGui::GetIO().MousePos.y < maximum.y;
  const bool pointer_activated = ImGui::IsItemClicked(ImGuiMouseButton_Left) &&
                                 !pointer_over_trailing && !expansion_changed;
  const bool keyboard_activated =
      interaction.focused &&
      (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
       ImGui::IsKeyPressed(ImGuiKey_Space, false)) &&
      !expansion_changed;
  detail::EndAvailability(spec.availability, spec.tooltip);
  if (spec.expandable && native_open) {
    ++tree.open_nodes_;
  }

  detail::ControlColors colors = detail::ResolveControlColors({
      .disabled = disabled,
      .selected = spec.selected,
      .invalid = spec.status == SemanticStatus::Failure,
      .hovered = interaction.hovered,
      .pressed = interaction.active,
      .focused = interaction.focused,
  });
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  DrawTreeConnector(draw_list, depth, node_cursor, minimum, maximum, row_height,
                    metrics,
                    disabled ? palette.text_disabled : palette.text_secondary);
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

  float text_x = node_cursor.x + ImGui::GetTreeNodeToLabelSpacing();
  const float center_y = (minimum.y + maximum.y) * 0.5f;

  if (spec.leading_icon) {
    const float icon_size = Scale(16.0f);
    spec.leading_icon(
        {
            .minimum = {.x = text_x, .y = center_y - icon_size * 0.5f},
            .maximum = {.x = text_x + icon_size,
                        .y = center_y + icon_size * 0.5f},
        },
        disabled ? palette.text_disabled : StatusForeground(spec.status));
    text_x += metrics.geometry.icon + metrics.spacing.space03;
  }

  const ColorRgba label_color = disabled ? palette.text_disabled
                                : spec.status == SemanticStatus::Neutral
                                    ? colors.text
                                    : StatusForeground(spec.status);
  ImFont *font = ImGui::GetFont();
  const float label_font_size = Scale(21.0f);
  const float secondary_font_size = Scale(16.0f);
  const ImVec2 label_size = font->CalcTextSizeA(
      label_font_size, std::numeric_limits<float>::max(), 0.0f, label.c_str());
  const ImVec2 secondary_size = font->CalcTextSizeA(
      secondary_font_size, std::numeric_limits<float>::max(), 0.0f,
      secondary_label.c_str());
  const float copy_height =
      secondary_label.empty()
          ? label_size.y
          : label_size.y + metrics.spacing.space01 + secondary_size.y;
  const float text_y = std::floor(center_y - copy_height * 0.5f);
  draw_list->PushClipRect(
      ImVec2(text_x, minimum.y),
      ImVec2(maximum.x - trailing_width - Scale(4.0f), maximum.y), true);
  draw_list->AddText(font, label_font_size, ImVec2(text_x, text_y),
                     ImGui::GetColorU32(ToImVec4(label_color)), label.c_str());
  if (!secondary_label.empty()) {
    const ColorRgba secondary_color = disabled ? palette.text_disabled
                                      : spec.status == SemanticStatus::Neutral
                                          ? palette.text_secondary
                                          : StatusForeground(spec.status);
    draw_list->AddText(
        font, secondary_font_size,
        ImVec2(text_x, text_y + label_size.y + metrics.spacing.space01),
        ImGui::GetColorU32(ToImVec4(secondary_color)), secondary_label.c_str());
  }
  draw_list->PopClipRect();

  std::optional<detail::ScopedInteractionPreview> inline_preview;
  if (detail::CurrentInteractionPreview().has_value()) {
    inline_preview.emplace(detail::InteractionPreview::Rest);
  }
  ImGui::PushID(id.c_str());
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

  ImGui::PopID();
  if (trailing_width > 0.0f) {
    // Inline targets temporarily move the layout cursor back over the row.
    // Submit a zero-size item so ImGui accepts the restored boundary, offset
    // by ItemSpacing so the submission does not add a second row gap.
    ImGui::SetCursorScreenPos(ImVec2(
        cursor_after.x, cursor_after.y - ImGui::GetStyle().ItemSpacing.y));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
  }

  HierarchyRowResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = (pointer_activated || keyboard_activated) && !disabled &&
                     !color_activated && !action_activated &&
                     !visibility_changed;
  result.additive = result.activated && ImGui::GetIO().KeyCtrl;
  result.range = result.activated && ImGui::GetIO().KeyShift;
  result.expansion_changed = expansion_changed;
  result.expanded = spec.expandable && native_open;
  result.color_activated = color_activated && !disabled;
  result.action_activated = action_activated && !disabled;
  result.visibility_changed = visibility_changed && !disabled;
  result.visibility = visibility;
  return result;
}

InformationTree::InformationTree() {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float vertical_padding =
      std::max((metrics.inspector.information_row_minimum_height -
                ImGui::GetFontSize()) *
                   0.5f,
               0.0f);
  const ImGuiStyle &style = ImGui::GetStyle();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(style.ItemSpacing.x, metrics.spacing.condensed));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(metrics.spacing.space03, vertical_padding));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing,
                      metrics.explorer.tree_indent);
}

InformationTree::~InformationTree() {
  const bool unbalanced = open_nodes_ != 0;
  while (open_nodes_ > 0) {
    ImGui::TreePop();
    --open_nodes_;
  }
  ImGui::PopStyleVar(3);
  IM_ASSERT(!unbalanced &&
            "Every expanded information row must have a matching Pop()");
}

void InformationTree::Pop() {
  IM_ASSERT(open_nodes_ > 0 &&
            "Cannot pop an information tree with no open parent");
  if (open_nodes_ <= 0) {
    return;
  }
  ImGui::TreePop();
  --open_nodes_;
}

InformationTreeRowResult
InformationTreeRow(InformationTree &tree, const InformationTreeRowSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  const std::string value = detail::Owned(spec.value);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const int depth = tree.open_nodes_;
  const float visibility_width =
      spec.visibility.has_value()
          ? metrics.explorer.audit_visibility_column_width
          : 0.0f;
  const ImVec2 node_cursor = ImGui::GetCursorScreenPos();
  const ImVec2 content_min(ImGui::GetWindowPos().x +
                               ImGui::GetWindowContentRegionMin().x,
                           node_cursor.y);
  const ImVec2 content_max(
      ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x,
      node_cursor.y + metrics.inspector.information_row_minimum_height);
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(content_min, content_max,
                           ImGui::GetColorU32(ToImVec4(palette.surface_muted)));

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow |
      ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding |
      ImGuiTreeNodeFlags_NavLeftJumpsToParent;
  if (!spec.expandable) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  } else {
    ImGui::SetNextItemOpen(spec.expanded, ImGuiCond_Always);
  }

  const ColorRgba foreground = disabled ? palette.text_disabled
                               : spec.status == SemanticStatus::Neutral
                                   ? palette.text_primary
                                   : StatusForeground(spec.status);
  const std::string native_id = "##" + id;
  detail::BeginAvailability(spec.availability);
  ImGui::SetNextItemAllowOverlap();
  ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(foreground));
  const bool native_open = ImGui::TreeNodeEx(native_id.c_str(), flags);
  ImGui::PopStyleColor();
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const ImVec2 cursor_after = ImGui::GetCursorScreenPos();
  const bool expansion_changed = spec.expandable && ImGui::IsItemToggledOpen();
  detail::EndAvailability(spec.availability, {});
  if (spec.expandable && native_open) {
    ++tree.open_nodes_;
  }

  draw_list->AddRect(minimum, maximum,
                     ImGui::GetColorU32(ToImVec4(palette.border)), 0.0f, 0,
                     metrics.geometry.border);
  DrawTreeConnector(draw_list, depth, node_cursor, minimum, maximum,
                    metrics.inspector.information_row_minimum_height, metrics,
                    disabled ? palette.text_disabled : palette.text_secondary);
  detail::DrawFocusRing(interaction);

  ImFont *font = ImGui::GetFont();
  const float font_size = Scale(21.0f);
  const ImVec2 label_size = font->CalcTextSizeA(
      font_size, std::numeric_limits<float>::max(), 0.0f, label.c_str());
  const ImVec2 value_size = font->CalcTextSizeA(
      font_size, std::numeric_limits<float>::max(), 0.0f, value.c_str());
  const float text_y =
      std::floor((minimum.y + maximum.y - label_size.y) * 0.5f);
  const float label_x = node_cursor.x + ImGui::GetTreeNodeToLabelSpacing();
  const float value_x =
      maximum.x - visibility_width - metrics.spacing.space03 - value_size.x;
  draw_list->PushClipRect(
      ImVec2(label_x, minimum.y),
      ImVec2(std::max(label_x, value_x - metrics.spacing.space03), maximum.y),
      true);
  draw_list->AddText(font, font_size, ImVec2(label_x, text_y),
                     ImGui::GetColorU32(ToImVec4(foreground)), label.c_str());
  draw_list->PopClipRect();
  draw_list->AddText(font, font_size, ImVec2(value_x, text_y),
                     ImGui::GetColorU32(ToImVec4(foreground)), value.c_str());

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
    ImGui::PushID(id.c_str());
    const InlineTargetResult target = IconTarget(
        "##visibility",
        ImVec2(maximum.x - visibility_width + Scale(2.0f),
               (minimum.y + maximum.y) * 0.5f - Scale(12.0f)),
        icon, disabled ? palette.text_disabled : palette.text_secondary,
        visibility_tooltip, spec.availability);
    ImGui::PopID();
    visibility_changed = target.activated;
    if (visibility == ToggleState::Mixed) {
      draw_list->AddLine(
          ImVec2(target.minimum.x + Scale(7.0f),
                 target.maximum.y - Scale(4.0f)),
          ImVec2(target.maximum.x - Scale(7.0f),
                 target.maximum.y - Scale(4.0f)),
          ImGui::GetColorU32(ToImVec4(disabled ? palette.text_disabled
                                               : palette.text_primary)),
          Scale(2.0f));
    }
    if (visibility_changed) {
      visibility = NextVisibilityState(visibility);
    }
    ImGui::SetCursorScreenPos(ImVec2(
        cursor_after.x, cursor_after.y - ImGui::GetStyle().ItemSpacing.y));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
  }

  InformationTreeRowResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.expansion_changed = expansion_changed;
  result.expanded = spec.expandable && native_open;
  result.visibility_changed = visibility_changed && !disabled;
  result.visibility = visibility;
  return result;
}

} // namespace fancy_ui
