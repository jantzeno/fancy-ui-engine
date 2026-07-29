#pragma once

#include <functional>
#include <string_view>

namespace fancy_ui::shell {

using DrawCallback = std::function<void()>;

struct RegionSpec {
  std::string_view id;
  DrawCallback draw;
  bool visible = true;
  bool menu_bar = false;
  bool zero_padding = false;
};

} // namespace fancy_ui::shell
