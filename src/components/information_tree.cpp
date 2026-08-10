#include "fancy_ui/components/information_tree.hpp"

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

} // namespace

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
          ? metrics.geometry.icon * 3.0f + metrics.spacing.space03 + Scale(2.0f)
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
        .off_icon = spec.hidden_icon,
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
