#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/choice.hpp"

#include <cstddef>
#include <functional>
#include <span>
#include <string_view>

namespace fancy_ui {

struct TabSetSpec {
  std::string_view id;
  std::span<const ChoiceSpec> tabs;
  std::size_t selected_index = 0;
  bool request_focus = false;
  float width = 0.0f;
  std::function<void(std::size_t)> draw_panel;
};

struct TabSetResult : InteractionResult {
  bool changed = false;
  std::size_t selected_index = 0;
};

[[nodiscard]] TabSetResult TabSet(const TabSetSpec &spec);

} // namespace fancy_ui
