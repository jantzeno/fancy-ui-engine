#pragma once

#include "fancy_ui/component_types.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace fancy_ui {

struct ExplorerSearchSpec {
  std::string_view id;
  std::string_view placeholder = "Filter";
  std::string_view query;
  std::size_t capacity = 512;
  Availability availability;
};

struct ExplorerSearchResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  std::string query;
};

[[nodiscard]] ExplorerSearchResult
ExplorerSearch(const ExplorerSearchSpec &spec);

} // namespace fancy_ui
