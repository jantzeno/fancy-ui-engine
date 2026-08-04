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

const CommandView *FindCommand(const std::vector<CommandView> &commands,
                               const CommandId command) {
  for (const CommandView &candidate : commands) {
    if (candidate.command == command) {
      return &candidate;
    }
  }
  return nullptr;
}

const CommandView *FindToolbarCommand(const std::vector<ToolbarItemView> &items,
                                      const CommandId command) {
  for (const ToolbarItemView &item : items) {
    if (const CommandView *candidate = std::get_if<CommandView>(&item);
        candidate != nullptr && candidate->command == command) {
      return candidate;
    }
    const ToolbarPopoverView *popover = std::get_if<ToolbarPopoverView>(&item);
    if (popover == nullptr) {
      continue;
    }
    for (const ToolbarPopoverItemView &popover_item : popover->items) {
      if (const CommandView *candidate =
              std::get_if<CommandView>(&popover_item);
          candidate != nullptr && candidate->command == command) {
        return candidate;
      }
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

const CommandView *FindCommand(const ApplicationView &view,
                               const CommandId command) {
  if (const CommandView *found = FindMenuCommand(view.application_bar, command);
      found != nullptr) {
    return found;
  }
  if (const CommandView *found =
          FindToolbarCommand(view.context_toolbar.items, command);
      found != nullptr) {
    return found;
  }
  if (const CommandView *found =
          FindToolbarCommand(view.workspace.viewport_toolbar, command);
      found != nullptr) {
    return found;
  }
  if (const CommandView *found = FindCommand(view.explorer.commands, command);
      found != nullptr) {
    return found;
  }
  if (const CommandView *found =
          FindCommand(view.inspector.primary_commands, command);
      found != nullptr) {
    return found;
  }
  for (const SectionView &section : view.inspector.sections) {
    if (const CommandView *found = FindCommand(section.commands, command);
        found != nullptr) {
      return found;
    }
  }
  if (view.operation.has_value()) {
    if (const CommandView *found =
            FindCommand(view.operation->commands, command);
        found != nullptr) {
      return found;
    }
  }
  return nullptr;
}

} // namespace fancy_ui::steppenface
