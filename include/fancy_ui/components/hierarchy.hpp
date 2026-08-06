#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace fancy_ui {

/**
 * Resolves descendant visibility for a parent hierarchy row.
 *
 * Empty groups resolve Off. Any disagreement, including an already mixed
 * descendant, resolves Mixed.
 */
[[nodiscard]] ToggleState
AggregateVisibility(std::span<const ToggleState> descendants);

/**
 * Returns the value produced by activating a visibility control.
 *
 * On becomes Off. Off and Mixed become On so one action can reveal an entire
 * partially hidden group.
 */
[[nodiscard]] ToggleState NextVisibilityState(ToggleState current);

struct HierarchyRowSpec {
  std::string_view id;
  std::string_view label;
  std::string_view secondary_label;
  std::string_view tooltip;
  bool expandable = false;
  bool expanded = false;
  bool selected = false;
  SemanticStatus status = SemanticStatus::Neutral;
  IconPainter leading_icon;
  std::optional<ColorRgba> color;
  std::string_view color_tooltip = "Edit color";
  bool request_color_focus = false;
  IconPainter action_icon;
  std::string_view action_tooltip = "Row actions";
  std::optional<ToggleState> visibility;
  IconPainter visible_icon;
  IconPainter hidden_icon;
  std::string_view visibility_tooltip;
  Availability availability;
};

struct HierarchyRowResult : InteractionResult {
  bool activated = false;
  bool additive = false;
  bool range = false;
  bool expansion_changed = false;
  bool expanded = false;
  bool color_activated = false;
  bool action_activated = false;
  bool visibility_changed = false;
  ToggleState visibility = ToggleState::Off;
};

/**
 * Scopes one structurally nested hierarchy.
 *
 * The scope keeps native ImGui tree pushes balanced without exposing ImGui
 * types through the public API. Draw an expanded row's children immediately
 * after it, then call Pop() once before drawing its next sibling.
 */
class HierarchyTree {
public:
  HierarchyTree();
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
};

/**
 * Draws one full-width hierarchy row with independent inline actions.
 *
 * Inline color, overflow, and visibility targets suppress row activation so
 * one pointer gesture has exactly one owner. ImGui derives indentation and
 * connector lines from the order of HierarchyRow() and HierarchyTree::Pop().
 */
[[nodiscard]] HierarchyRowResult HierarchyRow(HierarchyTree &tree,
                                              const HierarchyRowSpec &spec);

struct InformationTreeRowSpec {
  std::string_view id;
  std::string_view label;
  std::string_view value;
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

/** Scopes one non-selectable Inspector information tree. */
class InformationTree {
public:
  InformationTree();
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
};

/**
 * Draws an Inspector information row with disclosure and aggregate visibility.
 *
 * Unlike HierarchyRow(), the row has no Explorer selection or review-action
 * behavior.
 */
[[nodiscard]] InformationTreeRowResult
InformationTreeRow(InformationTree &tree, const InformationTreeRowSpec &spec);

} // namespace fancy_ui
