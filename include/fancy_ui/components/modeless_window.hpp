#pragma once

#include "fancy_ui/component_types.hpp"

#include <functional>
#include <string_view>

namespace fancy_ui {

struct ModelessWindowState {
  bool open = false;
  bool was_open = false;
  bool restore_focus = false;
  Vec2 position;
  Vec2 size;
};

struct ModelessWindowSpec {
  std::string_view id;
  std::string_view title;
  bool request_open = false;
  bool request_close = false;
  Vec2 initial_size{640.0f, 480.0f};
  Vec2 minimum_size{320.0f, 240.0f};
  Vec2 maximum_size{1600.0f, 1200.0f};
  std::function<void()> draw_content;
  std::function<void()> draw_footer;
};

struct ModelessWindowResult {
  ModelessWindowState state;
  bool opened = false;
  bool closed = false;
  bool geometry_changed = false;
};

[[nodiscard]] ModelessWindowResult
ModelessWindow(const ModelessWindowSpec &spec, ModelessWindowState state);

} // namespace fancy_ui
