#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/choice.hpp"

#include <cstddef>
#include <span>
#include <string_view>

namespace fancy_ui {

struct SegmentedControlSpec {
  std::string_view id;
  std::span<const ChoiceSpec> choices;
  std::size_t selected_index = 0;
  float width = 0.0f;
};

struct SegmentedControlResult : InteractionResult {
  bool changed = false;
  std::size_t selected_index = 0;
};

[[nodiscard]] SegmentedControlResult
SegmentedControl(const SegmentedControlSpec &spec);

} // namespace fancy_ui
