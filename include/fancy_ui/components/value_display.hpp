#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

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
