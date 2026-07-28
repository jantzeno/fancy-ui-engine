#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

struct HierarchyRowSpec {
  std::string_view id;
  std::string_view label;
  std::string_view secondary_label;
  std::string_view tooltip;
  int depth = 0;
  bool expandable = false;
  bool expanded = false;
  bool selected = false;
  SemanticStatus status = SemanticStatus::Neutral;
  std::optional<ColorRgba> color;
  IconPainter action_icon;
  std::optional<ToggleState> visibility;
  IconPainter visible_icon;
  IconPainter hidden_icon;
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
 * Draws one full-width hierarchy row with independent inline actions.
 *
 * Inline color, overflow, and visibility targets suppress row activation so
 * one pointer gesture has exactly one owner.
 */
[[nodiscard]] HierarchyRowResult HierarchyRow(const HierarchyRowSpec &spec);

} // namespace fancy_ui
