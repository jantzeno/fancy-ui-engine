#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

enum class DisclosureRowVariant {
  Item,
  PanelHeader,
};

struct DisclosureRowSpec {
  std::string_view id;
  std::string_view label;
  std::string_view metadata;
  std::string_view tooltip;
  DisclosureRowVariant variant = DisclosureRowVariant::Item;
  bool expandable = false;
  bool expanded = false;
  bool selected = false;
  FontHandle font;
  IconPainter leading_icon;
  SemanticStatus status = SemanticStatus::Neutral;
  float reserved_trailing_width = 0.0f;
  Availability availability;
};

struct DisclosureRowResult : InteractionResult {
  bool activated = false;
  bool expansion_changed = false;
  bool expanded = false;
};

[[nodiscard]] DisclosureRowResult DisclosureRow(const DisclosureRowSpec &spec);

} // namespace fancy_ui
