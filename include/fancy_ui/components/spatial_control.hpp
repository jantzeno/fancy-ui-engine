#pragma once

#include <string_view>

namespace fancy_ui {

struct SpatialControlSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  bool selected = false;
};

struct SpatialControlResult {
  bool activated = false;
};

[[nodiscard]] SpatialControlResult
SpatialControl(const SpatialControlSpec &spec);

} // namespace fancy_ui
