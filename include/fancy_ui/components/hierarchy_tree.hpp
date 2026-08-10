#pragma once

#include "fancy_ui/component_types.hpp"

#include <span>

namespace fancy_ui {

struct HierarchyRowSpec;
struct HierarchyRowResult;

struct HierarchyTreeStyle {
  FontHandle section_font;
};

[[nodiscard]] ToggleState
AggregateVisibility(std::span<const ToggleState> descendants);
[[nodiscard]] ToggleState NextVisibilityState(ToggleState current);

class HierarchyTree {
public:
  explicit HierarchyTree(HierarchyTreeStyle style = {});
  ~HierarchyTree();

  HierarchyTree(const HierarchyTree &) = delete;
  HierarchyTree &operator=(const HierarchyTree &) = delete;
  HierarchyTree(HierarchyTree &&) = delete;
  HierarchyTree &operator=(HierarchyTree &&) = delete;

  void Pop();

private:
  friend HierarchyRowResult HierarchyRow(HierarchyTree &tree,
                                         const HierarchyRowSpec &spec);

  int open_nodes_ = 0;
  FontHandle section_font_;
};

} // namespace fancy_ui
