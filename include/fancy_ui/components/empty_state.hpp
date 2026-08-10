#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct EmptyStateSpec {
  std::string_view id;
  std::string_view title;
  std::string_view message;
  IconPainter icon;
  float minimum_height = 44.0f;
};

void EmptyState(const EmptyStateSpec &spec);

} // namespace fancy_ui
