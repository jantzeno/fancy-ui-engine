#include "gallery_hierarchy_studies.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace fancy_ui::gallery {

namespace {

struct HierarchyStudyDefinition {
  const char *title;
  const char *description;
};

constexpr HierarchyStudyDefinition kApprovedStudy{
    .title = "Sectioned outliner · approved",
    .description = "Strong roots, native indentation, and no connector lines.",
};

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

void DrawStudyNode(detail::UiAssetAtlas &assets, HierarchyStudyState &state,
                   const std::size_t index, const int depth) {
  const HierarchyStudyNode &node = kHierarchyStudyNodes[index];
  const bool has_children = HierarchyStudyHasChildren(index);
  const bool section_root = depth == 0;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
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

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow |
      ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_NavLeftJumpsToParent;
  if (state.selected == index) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  if (!has_children) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  } else {
    ImGui::SetNextItemOpen(state.expanded[index], ImGuiCond_Always);
  }
  flags |= ImGuiTreeNodeFlags_DrawLinesNone;

  const std::string native_id = "##" + std::string(node.id);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(metrics.spacing.space03, vertical_padding));
  ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(palette.text_secondary));
  const bool native_open = ImGui::TreeNodeEx(native_id.c_str(), flags);
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const bool expansion_changed = has_children && ImGui::IsItemToggledOpen();
  if (expansion_changed) {
    state.expanded[index] = native_open;
  }
  const bool keyboard_activated =
      interaction.focused && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                              ImGui::IsKeyPressed(ImGuiKey_Space, false));
  if (!expansion_changed &&
      (ImGui::IsItemClicked(ImGuiMouseButton_Left) || keyboard_activated)) {
    state.selected = index;
  }

  if (state.selected == index) {
    ImGui::GetWindowDrawList()->AddRectFilled(
        minimum, ImVec2(minimum.x + Scale(3.0f), maximum.y),
        ImGui::GetColorU32(ToImVec4(palette.focus)));
  }
  detail::DrawFocusRing(interaction, false, 0.0f);

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const float center_y = (minimum.y + maximum.y) * 0.5f;
  float text_x = node_cursor.x + ImGui::GetTreeNodeToLabelSpacing();
  const float icon_size = metrics.geometry.icon;
  static_cast<void>(assets.DrawIcon(
      node.icon, steppenface::UiIconSize::Small16,
      {.minimum = {.x = text_x, .y = center_y - icon_size * 0.5f},
       .maximum = {.x = text_x + icon_size, .y = center_y + icon_size * 0.5f}},
      palette.text_secondary));
  text_x += icon_size + metrics.spacing.space03;
  if (node.status != SemanticStatus::Neutral) {
    const float dot_size = Scale(8.0f);
    draw_list->AddCircleFilled(
        ImVec2(text_x + dot_size * 0.5f, center_y), dot_size * 0.5f,
        ImGui::GetColorU32(detail::StatusColor(node.status)));
    text_x += dot_size + metrics.spacing.space03;
  }

  ImFont *label_font = section_root && assets.bold_font() != nullptr
                           ? assets.bold_font()
                           : ImGui::GetFont();
  const float label_font_size = Scale(section_root ? 17.0f : 16.0f);
  const ImVec2 label_size = label_font->CalcTextSizeA(
      label_font_size, maximum.x - text_x, 0.0f, node.label.data(),
      node.label.data() + node.label.size());
  const float clip_maximum_x = maximum.x - metrics.spacing.space02;
  draw_list->PushClipRect(ImVec2(text_x, minimum.y),
                          ImVec2(clip_maximum_x, maximum.y), true);
  draw_list->AddText(label_font, label_font_size,
                     ImVec2(text_x, std::floor(center_y - label_size.y * 0.5f)),
                     ImGui::GetColorU32(ToImVec4(palette.text_primary)),
                     node.label.data(), node.label.data() + node.label.size());
  if (!node.secondary_label.empty()) {
    const float secondary_x = text_x + label_size.x + metrics.spacing.space03;
    draw_list->AddText(
        ImGui::GetFont(), Scale(14.0f),
        ImVec2(secondary_x, std::floor(center_y - ImGui::GetFontSize() * 0.5f)),
        ImGui::GetColorU32(ToImVec4(palette.text_secondary)),
        node.secondary_label.data(),
        node.secondary_label.data() + node.secondary_label.size());
  }
  draw_list->PopClipRect();

  if (has_children && native_open) {
    // ponytail: this fixed gallery study has 17 rows; add child ranges only if
    // it becomes runtime data.
    for (std::size_t child = 0; child < kHierarchyStudyNodes.size(); ++child) {
      if (kHierarchyStudyNodes[child].parent == static_cast<int>(index) &&
          HierarchyStudyIsVisible(child, state)) {
        DrawStudyNode(assets, state, child, depth + 1);
      }
    }
    ImGui::TreePop();
  }
}

void DrawStudyCard(detail::UiAssetAtlas &assets, HierarchyStudyState &state,
                   const HierarchyStudyDefinition &study) {
  const SemanticPalette &palette = CurrentPalette();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(palette.surface));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(Scale(12.0f), Scale(12.0f)));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
  if (ImGui::BeginChild(
          "##hierarchy-study-card", ImVec2(Scale(640.0f), Scale(720.0f)),
          ImGuiChildFlags_Borders,
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
    if (assets.bold_font() != nullptr) {
      ImGui::PushFont(assets.bold_font(), Scale(20.0f));
    }
    ImGui::TextUnformatted(study.title);
    if (assets.bold_font() != nullptr) {
      ImGui::PopFont();
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + Scale(610.0f));
    detail::DrawSecondaryText(study.description);
    ImGui::PopTextWrapPos();
    ImGui::Spacing();

    const ImGuiStyle &imgui_style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(imgui_style.ItemSpacing.x, 0.0f));
    for (std::size_t index = 0; index < kHierarchyStudyNodes.size(); ++index) {
      if (kHierarchyStudyNodes[index].parent != -1) {
        continue;
      }
      if (index != 0) {
        ImGui::Dummy(ImVec2(0.0f, Scale(4.0f)));
      }
      DrawStudyNode(assets, state, index, 0);
    }
    ImGui::PopStyleVar();
  }
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
}

} // namespace

void DrawHierarchyStudies(detail::UiAssetAtlas &assets,
                          HierarchyStudyState &state) {
  ImGui::PushFont(nullptr, Scale(21.0f));
  detail::DrawSecondaryText(
      "Approved hierarchy treatment for STEP, SVG, DXF, and Canvas Issues.");
  ImGui::PopFont();
  ImGui::Spacing();
  DrawStudyCard(assets, state, kApprovedStudy);
}

} // namespace fancy_ui::gallery
