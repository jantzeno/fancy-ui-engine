#pragma once

#include "fancy_ui/component_types.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace fancy_ui::detail {
class UiAssetAtlas;
}

namespace fancy_ui::gallery {

struct HierarchyStudyNode {
  std::string_view id;
  std::string_view label;
  std::string_view secondary_label;
  std::string_view icon;
  int parent = -1;
  SemanticStatus status = SemanticStatus::Neutral;
};

inline constexpr std::array<HierarchyStudyNode, 20> kHierarchyStudyNodes{{
    {.id = "step",
     .label = "STEP",
     .secondary_label = "2 parts",
     .icon = "model"},
    {.id = "step-front-housing",
     .label = "Front housing",
     .secondary_label = "Assembly",
     .icon = "model",
     .parent = 0},
    {.id = "step-face-plate",
     .label = "Face plate",
     .secondary_label = "Solid",
     .icon = "model",
     .parent = 1},
    {.id = "step-rear-bracket",
     .label = "Rear bracket",
     .secondary_label = "Solid",
     .icon = "model",
     .parent = 0},
    {.id = "svg", .label = "SVG", .secondary_label = "2 paths", .icon = "svg"},
    {.id = "svg-layer-1",
     .label = "Layer 1",
     .secondary_label = "2 paths",
     .icon = "objects",
     .parent = 4},
    {.id = "svg-outer-contour",
     .label = "Outer contour",
     .secondary_label = "Closed",
     .icon = "path",
     .parent = 5},
    {.id = "svg-open-contour",
     .label = "Open contour",
     .secondary_label = "Open",
     .icon = "path",
     .parent = 5,
     .status = SemanticStatus::Warning},
    {.id = "dxf",
     .label = "DXF",
     .secondary_label = "3 entities",
     .icon = "dxf"},
    {.id = "dxf-cut-layer",
     .label = "CUT",
     .secondary_label = "Geometry",
     .icon = "objects",
     .parent = 8},
    {.id = "dxf-line-12",
     .label = "Line 12",
     .secondary_label = "48.0 mm",
     .icon = "line",
     .parent = 9},
    {.id = "dxf-arc-4",
     .label = "Arc 4",
     .secondary_label = "R 18.0 mm",
     .icon = "arc",
     .parent = 9,
     .status = SemanticStatus::Warning},
    {.id = "dxf-circle-2",
     .label = "Circle 2",
     .secondary_label = "24.0 mm dia.",
     .icon = "circle",
     .parent = 9},
    {.id = "canvas-issues",
     .label = "Canvas Issues",
     .secondary_label = "10 issues",
     .icon = "alert",
     .status = SemanticStatus::Warning},
    {.id = "canvas-invalid",
     .label = "Invalid",
     .secondary_label = "1",
     .icon = "alert",
     .parent = 13,
     .status = SemanticStatus::Failure},
    {.id = "canvas-self-intersection",
     .label = "Self-intersection",
     .secondary_label = "1",
     .icon = "path",
     .parent = 14,
     .status = SemanticStatus::Failure},
    {.id = "canvas-repairable",
     .label = "Repairable",
     .secondary_label = "5",
     .icon = "information",
     .parent = 13,
     .status = SemanticStatus::Information},
    {.id = "canvas-open-contours",
     .label = "Open contours",
     .secondary_label = "5 paths",
     .icon = "path",
     .parent = 16,
     .status = SemanticStatus::Information},
    {.id = "canvas-warnings",
     .label = "Warnings",
     .secondary_label = "4",
     .icon = "alert",
     .parent = 13,
     .status = SemanticStatus::Warning},
    {.id = "canvas-ambiguous-cleanup",
     .label = "Ambiguous cleanup",
     .secondary_label = "4",
     .icon = "alert",
     .parent = 18,
     .status = SemanticStatus::Warning},
}};

struct HierarchyStudyState {
  std::array<bool, kHierarchyStudyNodes.size()> expanded{
      true,  true,  false, false, true, true,  false, false, true, true,
      false, false, false, true,  true, false, true,  false, true, false,
  };
  std::size_t selected = 6;
};

[[nodiscard]] constexpr bool
HierarchyStudyHasChildren(const std::size_t index) {
  for (const HierarchyStudyNode &node : kHierarchyStudyNodes) {
    if (node.parent == static_cast<int>(index)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] constexpr int HierarchyStudyDepth(std::size_t index) {
  int depth = 0;
  int parent = kHierarchyStudyNodes[index].parent;
  while (parent >= 0) {
    ++depth;
    index = static_cast<std::size_t>(parent);
    parent = kHierarchyStudyNodes[index].parent;
  }
  return depth;
}

[[nodiscard]] constexpr bool
HierarchyStudyIsVisible(std::size_t index, const HierarchyStudyState &state) {
  int parent = kHierarchyStudyNodes[index].parent;
  while (parent >= 0) {
    if (!state.expanded[static_cast<std::size_t>(parent)]) {
      return false;
    }
    index = static_cast<std::size_t>(parent);
    parent = kHierarchyStudyNodes[index].parent;
  }
  return true;
}

void DrawHierarchyStudies(detail::UiAssetAtlas &assets,
                          HierarchyStudyState &state);

} // namespace fancy_ui::gallery
