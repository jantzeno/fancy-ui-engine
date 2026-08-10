#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct VisibilityToggleSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  ToggleState state = ToggleState::Off;
  IconPainter visible_icon;
  IconPainter hidden_icon;
  Availability availability;
};

struct VisibilityToggleResult : InteractionResult {
  bool changed = false;
  ToggleState state = ToggleState::Off;
};

[[nodiscard]] VisibilityToggleResult
VisibilityToggle(const VisibilityToggleSpec &spec);

} // namespace fancy_ui
