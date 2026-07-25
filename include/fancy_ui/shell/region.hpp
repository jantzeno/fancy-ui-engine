#pragma once

#include <functional>
#include <string_view>

namespace fancy_ui::shell {

using DrawCallback = std::function<void()>;

struct RegionSpec {
  std::string_view id;
  DrawCallback draw;
  bool visible = true;
};

} // namespace fancy_ui::shell
