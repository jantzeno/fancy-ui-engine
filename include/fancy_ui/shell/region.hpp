#pragma once

#include <functional>
#include <optional>
#include <string_view>

namespace fancy_ui::shell {

using DrawCallback = std::function<void()>;

struct RegionSpec {
  std::string_view id;
  DrawCallback draw;
  bool visible = true;
  bool menu_bar = false;
  std::optional<float> padding;
};

} // namespace fancy_ui::shell
