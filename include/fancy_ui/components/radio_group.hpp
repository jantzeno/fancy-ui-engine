#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/select_option.hpp"

#include <cstddef>
#include <span>
#include <string_view>

namespace fancy_ui {

enum class RadioGroupLayout {
  Horizontal,
  Vertical,
};

struct RadioGroupSpec {
  std::string_view id;
  std::string_view label;
  std::span<const SelectOption> options;
  std::size_t selected_index = 0;
  RadioGroupLayout layout = RadioGroupLayout::Vertical;
  Availability availability;
};

struct RadioGroupResult : InteractionResult {
  bool changed = false;
  std::size_t selected_index = 0;
};

[[nodiscard]] RadioGroupResult RadioGroup(const RadioGroupSpec &spec);

} // namespace fancy_ui
