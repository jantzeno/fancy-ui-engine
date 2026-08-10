#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/select_option.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace fancy_ui {

struct RenamableSelectState {
  bool renaming = false;
  bool restore_focus = false;
  std::string original;
  std::string draft;
};

struct RenamableSelectSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::span<const SelectOption> options;
  std::size_t selected_index = 0;
  Availability availability;
  Availability rename_availability;
  Validation validation;
};

struct RenamableSelectResult : InteractionResult {
  bool selection_changed = false;
  std::size_t selected_index = 0;
  bool rename_started = false;
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  std::string value;
};

[[nodiscard]] RenamableSelectResult
RenamableSelect(const RenamableSelectSpec &spec, RenamableSelectState &state);

} // namespace fancy_ui
