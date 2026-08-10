#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/select_option.hpp"

#include <cstddef>
#include <span>
#include <string_view>

namespace fancy_ui {

struct SelectSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::span<const SelectOption> options;
  std::size_t selected_index = 0;
  Availability availability;
  Validation validation;
};

struct SelectResult : InteractionResult {
  bool changed = false;
  std::size_t selected_index = 0;
};

[[nodiscard]] SelectResult Select(const SelectSpec &spec);

} // namespace fancy_ui
