#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct CheckboxSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  bool checked = false;
  Availability availability;
};

struct CheckboxResult : InteractionResult {
  bool changed = false;
  bool value = false;
};

[[nodiscard]] CheckboxResult Checkbox(const CheckboxSpec &spec);

} // namespace fancy_ui
