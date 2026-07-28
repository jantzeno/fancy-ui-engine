#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct SliderSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::string_view unit;
  float value = 0.0f;
  float minimum = 0.0f;
  float maximum = 1.0f;
  std::string_view format = "%.3f";
  Availability availability;
  Validation validation;
};

struct SliderResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  float value = 0.0f;
};

[[nodiscard]] SliderResult Slider(const SliderSpec &spec);

} // namespace fancy_ui
