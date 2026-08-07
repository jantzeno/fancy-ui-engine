#include "fancy_ui/components/hierarchy.hpp"

#include "fancy_ui/components/checkbox.hpp"
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

HierarchyTree::HierarchyTree(const HierarchyTreeStyle style)
    : section_font_(style.section_font) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const ImGuiStyle &imgui_style = ImGui::GetStyle();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(imgui_style.ItemSpacing.x, 0.0f));
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
  const std::string metadata = detail::Owned(spec.metadata);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const bool section_root = tree.open_nodes_ == 0;
  const float row_height = metrics.geometry.row_height;
  const float vertical_padding =
      std::max((row_height - ImGui::GetFontSize()) * 0.5f, 0.0f);
  const ImVec2 node_cursor = ImGui::GetCursorScreenPos();
  if (section_root) {
    const ImVec2 background_maximum(node_cursor.x +
                                        ImGui::GetContentRegionAvail().x,
                                    node_cursor.y + row_height);
    ImGui::GetWindowDrawList()->AddRectFilled(
        node_cursor, background_maximum,
        ImGui::GetColorU32(ToImVec4(palette.surface_muted)));
  }
  const std::string native_id = "##" + id;
  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow |
      ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding |
      ImGuiTreeNodeFlags_NavLeftJumpsToParent |
      ImGuiTreeNodeFlags_DrawLinesNone;
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
  const float color_width =
      spec.color.has_value() ? metrics.geometry.compact_target : 0.0f;
  const float action_width =
      spec.action_icon ? metrics.geometry.compact_target : 0.0f;
  const float visibility_width =
      spec.visibility.has_value() ? metrics.geometry.compact_target : 0.0f;
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
  if (spec.selected) {
    draw_list->AddRectFilled(minimum,
                             ImVec2(minimum.x + Scale(3.0f), maximum.y),
                             ImGui::GetColorU32(ToImVec4(palette.focus)));
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
        disabled ? palette.text_disabled : palette.text_secondary);
    text_x += metrics.geometry.icon + metrics.spacing.space03;
  }

  if (spec.status != SemanticStatus::Neutral) {
    const float dot_size = Scale(8.0f);
    draw_list->AddCircleFilled(
        ImVec2(text_x + dot_size * 0.5f, center_y), dot_size * 0.5f,
        ImGui::GetColorU32(ToImVec4(disabled ? palette.text_disabled
                                             : StatusForeground(spec.status))));
    text_x += dot_size + metrics.spacing.space03;
  }

  const ColorRgba label_color = disabled ? palette.text_disabled : colors.text;
  ImFont *font = section_root && tree.section_font_
                     ? reinterpret_cast<ImFont *>(tree.section_font_.value)
                     : ImGui::GetFont();
  const float font_size = metrics.typography.body_font_height;
  const ImVec2 label_size = font->CalcTextSizeA(
      font_size, std::numeric_limits<float>::max(), 0.0f, label.c_str());
  const float text_y = std::floor(center_y - label_size.y * 0.5f);
  draw_list->PushClipRect(
      ImVec2(text_x, minimum.y),
      ImVec2(maximum.x - trailing_width - Scale(4.0f), maximum.y), true);
  draw_list->AddText(font, font_size, ImVec2(text_x, text_y),
                     ImGui::GetColorU32(ToImVec4(label_color)), label.c_str());
  if (!metadata.empty()) {
    const ColorRgba metadata_color =
        disabled ? palette.text_disabled : palette.text_secondary;
    const float metadata_x = text_x + label_size.x + metrics.spacing.space03;
    ImFont *metadata_font = ImGui::GetFont();
    const ImVec2 metadata_size = metadata_font->CalcTextSizeA(
        font_size, std::numeric_limits<float>::max(), 0.0f, metadata.c_str());
    draw_list->AddText(
        metadata_font, font_size,
        ImVec2(metadata_x, std::floor(center_y - metadata_size.y * 0.5f)),
        ImGui::GetColorU32(ToImVec4(metadata_color)), metadata.c_str());
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
    const IconPainter &icon =
        visibility == ToggleState::Off ? spec.hidden_icon : spec.visible_icon;
    const std::string visibility_tooltip =
        !spec.visibility_tooltip.empty()
            ? detail::Owned(spec.visibility_tooltip)
        : visibility == ToggleState::On    ? "Hide"
        : visibility == ToggleState::Mixed ? "Show all descendants"
                                           : "Show";
    const InlineTargetResult visibility_target = IconTarget(
        "##visibility", ImVec2(trailing_x, center_y - Scale(12.0f)), icon,
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

InformationTree::InformationTree(const HierarchyTreeStyle style)
    : section_font_(style.section_font) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float vertical_padding =
      std::max((metrics.inspector.information_row_minimum_height -
                ImGui::GetFontSize()) *
                   0.5f,
               0.0f);
  const ImGuiStyle &imgui_style = ImGui::GetStyle();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(imgui_style.ItemSpacing.x, 0.0f));
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
  const std::string metadata = detail::Owned(spec.metadata);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const bool section_root = tree.open_nodes_ == 0;
  const float visibility_width =
      spec.visibility.has_value()
          ? metrics.geometry.icon * 2.0f + metrics.spacing.space03 * 2.0f
          : 0.0f;
  const ImVec2 node_cursor = ImGui::GetCursorScreenPos();
  const ImVec2 content_min(ImGui::GetWindowPos().x +
                               ImGui::GetWindowContentRegionMin().x,
                           node_cursor.y);
  const ImVec2 content_max(
      ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x,
      node_cursor.y + metrics.inspector.information_row_minimum_height);
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(
      content_min, content_max,
      ImGui::GetColorU32(
          ToImVec4(section_root ? palette.surface_muted : palette.surface)));

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow |
      ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding |
      ImGuiTreeNodeFlags_NavLeftJumpsToParent |
      ImGuiTreeNodeFlags_DrawLinesNone;
  if (!spec.expandable) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  } else {
    ImGui::SetNextItemOpen(spec.expanded, ImGuiCond_Always);
  }

  const ColorRgba foreground = disabled ? palette.text_disabled
                               : spec.status != SemanticStatus::Neutral
                                   ? StatusForeground(spec.status)
                                   : palette.text_primary;
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
  detail::DrawFocusRing(interaction);

  ImFont *font = section_root && tree.section_font_
                     ? reinterpret_cast<ImFont *>(tree.section_font_.value)
                     : ImGui::GetFont();
  const float font_size = metrics.typography.body_font_height;
  const ImVec2 label_size = font->CalcTextSizeA(
      font_size, std::numeric_limits<float>::max(), 0.0f, label.c_str());
  const ImVec2 metadata_size = font->CalcTextSizeA(
      font_size, std::numeric_limits<float>::max(), 0.0f, metadata.c_str());
  const float text_y =
      std::floor((minimum.y + maximum.y - label_size.y) * 0.5f);
  float label_x = node_cursor.x + ImGui::GetTreeNodeToLabelSpacing();
  if (spec.status != SemanticStatus::Neutral) {
    const float dot_size = Scale(8.0f);
    draw_list->AddCircleFilled(
        ImVec2(label_x + dot_size * 0.5f, (minimum.y + maximum.y) * 0.5f),
        dot_size * 0.5f,
        ImGui::GetColorU32(ToImVec4(disabled ? palette.text_disabled
                                             : StatusForeground(spec.status))));
    label_x += dot_size + metrics.spacing.space03;
  }
  const float metadata_x =
      maximum.x - visibility_width - metrics.spacing.space03 - metadata_size.x;
  draw_list->PushClipRect(
      ImVec2(label_x, minimum.y),
      ImVec2(std::max(label_x, metadata_x - metrics.spacing.space03),
             maximum.y),
      true);
  draw_list->AddText(font, font_size, ImVec2(label_x, text_y),
                     ImGui::GetColorU32(ToImVec4(foreground)), label.c_str());
  draw_list->PopClipRect();
  draw_list->AddText(font, font_size, ImVec2(metadata_x, text_y),
                     ImGui::GetColorU32(ToImVec4(foreground)),
                     metadata.c_str());

  bool visibility_changed = false;
  ToggleState visibility = spec.visibility.value_or(ToggleState::Off);
  if (spec.visibility.has_value()) {
    const std::string visibility_tooltip =
        !spec.visibility_tooltip.empty()
            ? detail::Owned(spec.visibility_tooltip)
        : visibility == ToggleState::On    ? "Hide"
        : visibility == ToggleState::Mixed ? "Show all descendants"
                                           : "Show";
    ImGui::PushID(id.c_str());
    ImGui::SetCursorScreenPos(
        ImVec2(maximum.x - visibility_width,
               (minimum.y + maximum.y) * 0.5f - Scale(12.0f)));
    const CheckboxResult target = Checkbox({
        .id = "visibility",
        .tooltip = visibility_tooltip,
        .state = visibility,
        .on_icon = spec.visible_icon,
        .off_icon = visibility == ToggleState::Mixed ? spec.visible_icon
                                                     : spec.hidden_icon,
        .show_checkbox = true,
        .availability = spec.availability,
    });
    ImGui::PopID();
    visibility_changed = target.changed;
    if (visibility_changed) {
      visibility = target.state;
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
