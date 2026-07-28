#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct StatusTextSpec {
  std::string_view label;
  SemanticStatus status = SemanticStatus::Neutral;
};

void StatusText(const StatusTextSpec &spec);

struct EmptyStateSpec {
  std::string_view id;
  std::string_view title;
  std::string_view message;
  IconPainter icon;
  float minimum_height = 44.0f;
};

void EmptyState(const EmptyStateSpec &spec);

/**
 * Shows a label and one read-only value, truncating the value visually while
 * retaining the complete content in its keyboard-and-pointer tooltip.
 */
struct ValueDisplaySpec {
  std::string_view id;
  std::string_view label;
  std::string_view value;
  std::string_view tooltip;
  bool mixed = false;
  float label_width = 88.0f;
};

[[nodiscard]] InteractionResult ValueDisplay(const ValueDisplaySpec &spec);

} // namespace fancy_ui
