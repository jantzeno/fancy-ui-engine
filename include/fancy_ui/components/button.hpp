#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

enum class ButtonVariant {
  Primary,
  Secondary,
  Tertiary,
  Destructive,
};

struct ButtonSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  ButtonVariant variant = ButtonVariant::Secondary;
  Availability availability;
  Vec2 size = {0.0f, 32.0f};
};

struct ButtonResult : InteractionResult {
  bool activated = false;
};

[[nodiscard]] ButtonResult Button(const ButtonSpec &spec);

} // namespace fancy_ui
