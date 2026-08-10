#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace fancy_ui {

struct CheckedMultiselectOption {
  std::string_view id;
  std::string_view label;
  ToggleState state = ToggleState::Off;
  Availability availability;
};

struct CheckedMultiselectSpec {
  std::string_view id;
  std::string_view label;
  std::string_view summary;
  std::string_view tooltip;
  std::span<const CheckedMultiselectOption> options;
  bool request_open = false;
  Availability availability;
};

struct CheckedMultiselectResult : InteractionResult {
  bool changed = false;
  std::optional<std::string> option_id;
  ToggleState state = ToggleState::Off;
  bool popup_open = false;
};

[[nodiscard]] CheckedMultiselectResult
CheckedMultiselect(const CheckedMultiselectSpec &spec);

} // namespace fancy_ui
