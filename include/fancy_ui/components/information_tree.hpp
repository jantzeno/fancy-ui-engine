#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/button.hpp"
#include "fancy_ui/components/hierarchy_tree.hpp"
#include "fancy_ui/components/metric_row.hpp"

#include <optional>
#include <span>
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
  bool selected = false;
  bool highlighted = false;
  SemanticStatus status = SemanticStatus::Neutral;
  std::span<const MetricValue> metrics;
  std::span<const ButtonSpec> actions;
  std::optional<ToggleState> visibility;
  IconPainter visible_icon;
  IconPainter hidden_icon;
  std::string_view visibility_tooltip;
  Availability availability;
};

struct InformationTreeRowResult : InteractionResult {
  bool activated = false;
  bool additive = false;
  bool range = false;
  bool expansion_changed = false;
  bool expanded = false;
  bool visibility_changed = false;
  ToggleState visibility = ToggleState::Off;
  std::optional<std::size_t> activated_action;
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
