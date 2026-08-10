#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace fancy_ui {

enum class ContextMenuItemKind {
  Command,
  Separator,
  Submenu,
};

struct ContextMenuItemSpec {
  std::string_view id;
  std::string_view label;
  std::string_view shortcut;
  std::string_view tooltip;
  ContextMenuItemKind kind = ContextMenuItemKind::Command;
  bool selected = false;
  Availability availability;
  std::span<const ContextMenuItemSpec> children;
};

struct ContextMenuState {
  bool open = false;
  bool restore_focus = false;
};

struct ContextMenuSpec {
  std::string_view id;
  std::span<const ContextMenuItemSpec> items;
  bool request_open = false;
  std::optional<Vec2> anchor;
};

struct ContextMenuResult {
  bool opened = false;
  bool closed = false;
  bool menu_open = false;
  std::optional<std::string> activated_id;
};

[[nodiscard]] ContextMenuResult ContextMenu(const ContextMenuSpec &spec,
                                            ContextMenuState &state);

} // namespace fancy_ui
