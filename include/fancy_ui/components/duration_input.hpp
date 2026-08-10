#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct DurationSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  int hours = 0;
  int minutes = 0;
  Availability availability;
  Validation validation;
};

struct DurationResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  int hours = 0;
  int minutes = 0;
};

[[nodiscard]] DurationResult Duration(const DurationSpec &spec);

} // namespace fancy_ui
