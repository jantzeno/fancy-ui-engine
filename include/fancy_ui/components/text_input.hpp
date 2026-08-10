#pragma once

#include "fancy_ui/component_types.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace fancy_ui {

struct TextInputSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::string_view value;
  std::string_view placeholder;
  std::size_t capacity = 512;
  Availability availability;
  Validation validation;
};

struct TextInputResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  std::string value;
};

[[nodiscard]] TextInputResult TextInput(const TextInputSpec &spec);

} // namespace fancy_ui
