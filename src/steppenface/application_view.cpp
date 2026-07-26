#include "fancy_ui/steppenface/application_view.hpp"

namespace fancy_ui::steppenface {

namespace {

const CommandView *FindMenuCommand(const std::vector<MenuItemView> &items,
                                   const CommandId command) {
  for (const MenuItemView &item : items) {
    if (item.command.has_value() && item.command->command == command) {
      return &*item.command;
    }
    if (const CommandView *nested = FindMenuCommand(item.children, command);
        nested != nullptr) {
      return nested;
    }
  }
  return nullptr;
}

} // namespace

const CommandView *FindMenuCommand(const ApplicationBarView &application_bar,
                                   const CommandId command) {
  for (const ApplicationMenuView &menu : application_bar.menus) {
    if (const CommandView *found = FindMenuCommand(menu.items, command);
        found != nullptr) {
      return found;
    }
  }
  return nullptr;
}

} // namespace fancy_ui::steppenface
