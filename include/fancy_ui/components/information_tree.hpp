#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/hierarchy_tree.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

struct InformationTreeRowSpec;
struct InformationTreeRowResult;

struct InformationTreeRowSpec {
  std::string_view id;
  std::string_view label;
  std::string_view metadata;
  bool expandable = false;
  bool expanded = false;
  SemanticStatus status = SemanticStatus::Neutral;
  std::optional<ToggleState> visibility;
  IconPainter visible_icon;
  IconPainter hidden_icon;
  std::string_view visibility_tooltip;
  Availability availability;
};

struct InformationTreeRowResult : InteractionResult {
  bool expansion_changed = false;
  bool expanded = false;
  bool visibility_changed = false;
  ToggleState visibility = ToggleState::Off;
};

class InformationTree {
public:
  explicit InformationTree(HierarchyTreeStyle style = {});
  ~InformationTree();

  InformationTree(const InformationTree &) = delete;
  InformationTree &operator=(const InformationTree &) = delete;
  InformationTree(InformationTree &&) = delete;
  InformationTree &operator=(InformationTree &&) = delete;

  void Pop();

private:
  friend InformationTreeRowResult
  InformationTreeRow(InformationTree &tree, const InformationTreeRowSpec &spec);

  int open_nodes_ = 0;
  FontHandle section_font_;
};

[[nodiscard]] InformationTreeRowResult
InformationTreeRow(InformationTree &tree, const InformationTreeRowSpec &spec);

} // namespace fancy_ui
