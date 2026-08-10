#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct ChoiceSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  Availability availability;
};

} // namespace fancy_ui
