#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

struct NumericInputSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::string_view unit;
  double value = 0.0;
  std::optional<double> minimum;
  std::optional<double> maximum;
  std::string_view format = "%.3f";
  Availability availability;
  Validation validation;
};

struct NumericInputResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  double value = 0.0;
};

[[nodiscard]] NumericInputResult NumericInput(const NumericInputSpec &spec);

} // namespace fancy_ui
