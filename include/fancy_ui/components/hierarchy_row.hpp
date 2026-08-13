#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/hierarchy_tree.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

struct HierarchyRowSpec {
  std::string_view id;
  std::string_view label;
  std::string_view metadata;
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
  bool drag_source = false;
  bool drop_target = false;
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
  bool drag_started = false;
  bool drop_received = false;
};

[[nodiscard]] HierarchyRowResult HierarchyRow(HierarchyTree &tree,
                                              const HierarchyRowSpec &spec);

} // namespace fancy_ui
