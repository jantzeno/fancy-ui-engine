#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct CheckboxSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  ToggleState state = ToggleState::Off;
  IconPainter on_icon;
  IconPainter off_icon;
  bool show_checkbox = true;
  Availability availability;
  Validation validation;
};

struct CheckboxResult : InteractionResult {
  bool changed = false;
  ToggleState state = ToggleState::Off;
};

[[nodiscard]] CheckboxResult Checkbox(const CheckboxSpec &spec);

} // namespace fancy_ui
