#include "fancy_ui/components/hierarchy_tree.hpp"

#include "fancy_ui/layout_metrics.hpp"

#include <imgui.h>

namespace fancy_ui {

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

} // namespace fancy_ui
