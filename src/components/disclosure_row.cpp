#include "fancy_ui/components/disclosure_row.hpp"

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

DisclosureRowResult DisclosureRow(const DisclosureRowSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  const std::string metadata = detail::Owned(spec.metadata);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const bool panel_header = spec.variant == DisclosureRowVariant::PanelHeader;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const float vertical_padding = std::max(
      (metrics.geometry.row_height - ImGui::GetFontSize()) * 0.5f, 0.0f);
  const ImVec2 node_cursor = ImGui::GetCursorScreenPos();
  if (panel_header) {
    ImGui::GetWindowDrawList()->AddRectFilled(
        node_cursor,
        ImVec2(node_cursor.x + ImGui::GetContentRegionAvail().x,
               node_cursor.y + metrics.geometry.row_height),
        ImGui::GetColorU32(ToImVec4(palette.surface_muted)));
  }

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow |
      ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding |
      ImGuiTreeNodeFlags_NavLeftJumpsToParent |
      ImGuiTreeNodeFlags_DrawLinesNone | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  if (spec.selected) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  if (!spec.expandable) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  } else {
    ImGui::SetNextItemOpen(spec.expanded, ImGuiCond_Always);
  }

  ImGui::PushID(id.c_str());
  detail::BeginAvailability(spec.availability);
  ImGui::SetNextItemAllowOverlap();
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(metrics.spacing.space03, vertical_padding));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                      panel_header ? 0.0f : metrics.geometry.control_radius);
  if (panel_header) {
    ImGui::PushStyleColor(ImGuiCol_Header, ToImVec4(palette.surface_muted));
  }
  ImGui::PushStyleColor(
      ImGuiCol_Text,
      ToImVec4(disabled ? palette.text_disabled : palette.text_secondary));
  const bool native_open = ImGui::TreeNodeEx("##row", flags);
  ImGui::PopStyleColor(panel_header ? 2 : 1);
  ImGui::PopStyleVar(2);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const float trailing_width = Scale(spec.reserved_trailing_width);
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

  const detail::ControlColors colors = detail::ResolveControlColors({
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
    const float icon_size = metrics.geometry.icon;
    spec.leading_icon(
        {.minimum = {.x = text_x, .y = center_y - icon_size * 0.5f},
         .maximum = {.x = text_x + icon_size,
                     .y = center_y + icon_size * 0.5f}},
        disabled ? palette.text_disabled : palette.text_secondary);
    text_x += icon_size + metrics.spacing.space03;
  }
  if (spec.status != SemanticStatus::Neutral) {
    const float dot_size = Scale(8.0f);
    draw_list->AddCircleFilled(
        ImVec2(text_x + dot_size * 0.5f, center_y), dot_size * 0.5f,
        ImGui::GetColorU32(ToImVec4(disabled ? palette.text_disabled
                                             : StatusForeground(spec.status))));
    text_x += dot_size + metrics.spacing.space03;
  }

  ImFont *font = spec.font ? reinterpret_cast<ImFont *>(spec.font.value)
                           : ImGui::GetFont();
  const float font_size = metrics.typography.body_font_height;
  const ImVec2 label_size = font->CalcTextSizeA(
      font_size, std::numeric_limits<float>::max(), 0.0f, label.c_str());
  const float text_y = std::floor(center_y - label_size.y * 0.5f);
  draw_list->PushClipRect(
      ImVec2(text_x, minimum.y),
      ImVec2(std::max(text_x, maximum.x - trailing_width - Scale(4.0f)),
             maximum.y),
      true);
  draw_list->AddText(font, font_size, ImVec2(text_x, text_y),
                     ImGui::GetColorU32(ToImVec4(
                         disabled ? palette.text_disabled : colors.text)),
                     label.c_str());
  if (!metadata.empty()) {
    const ImVec2 metadata_size = ImGui::CalcTextSize(metadata.c_str());
    draw_list->AddText(
        ImGui::GetFont(), font_size,
        ImVec2(text_x + label_size.x + metrics.spacing.space03,
               std::floor(center_y - metadata_size.y * 0.5f)),
        ImGui::GetColorU32(ToImVec4(disabled ? palette.text_disabled
                                             : palette.text_secondary)),
        metadata.c_str());
  }
  draw_list->PopClipRect();
  ImGui::PopID();

  DisclosureRowResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = (pointer_activated || keyboard_activated) && !disabled;
  result.expansion_changed = expansion_changed;
  result.expanded = spec.expandable && native_open;
  return result;
}

} // namespace fancy_ui
