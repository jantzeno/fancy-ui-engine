#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct NavigationItemSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  bool selected = false;
  Availability availability;
};

struct NavigationItemResult : InteractionResult {
  bool activated = false;
};

[[nodiscard]] NavigationItemResult
NavigationItem(const NavigationItemSpec &spec);

} // namespace fancy_ui
