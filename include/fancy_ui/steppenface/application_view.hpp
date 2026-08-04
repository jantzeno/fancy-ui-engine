#pragma once

#include "fancy_ui/steppenface/ui_types.hpp"
#include "fancy_ui/theme.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fancy_ui::steppenface {

enum class MenuItemKind : std::uint8_t {
  Command,
  Separator,
  Submenu,
  Workspace,
};

struct MenuItemView {
  UiId id;
  MenuItemKind kind = MenuItemKind::Command;
  std::string label;
  std::optional<CommandView> command;
  std::optional<WorkspaceKind> workspace;
  std::vector<MenuItemView> children;
};

struct ApplicationMenuView {
  UiId id;
  std::string label;
  std::vector<MenuItemView> items;
};

struct ApplicationBarView {
  std::vector<ApplicationMenuView> menus;
  WorkspaceKind active_workspace = WorkspaceKind::Model3d;
  bool document_dirty = true;
  std::string dirty_label = "Unsaved";
};

[[nodiscard]] const CommandView *
FindMenuCommand(const ApplicationBarView &application_bar, CommandId command);

struct ControlActionView {
  UiId field;
  FieldValue value;
  std::optional<UiId> target;
  Availability availability;
};

struct ToolbarChoiceView {
  UiId id;
  std::string label;
  std::string icon;
  std::string tooltip;
  bool selected = false;
  ControlActionView action;
};

struct ToolbarSegmentedView {
  UiId id;
  std::vector<ToolbarChoiceView> choices;
};

struct ToolbarActionView {
  UiId id;
  std::string label;
  std::string icon;
  std::string tooltip;
  bool selected = false;
  ControlActionView action;
};

struct ToolbarSeparatorView {
  UiId id;
};

struct ToolbarSpacerView {
  UiId id;
};

struct ToolbarMenuItemView {
  UiId id;
  std::string label;
  std::string secondary_label;
  bool selected = false;
  bool separator_before = false;
  ControlActionView action;
};

using ToolbarPopoverItemView = std::variant<ToolbarMenuItemView, CommandView>;

struct ToolbarPopoverView {
  UiId id;
  std::string label;
  std::string icon;
  std::string tooltip;
  Availability availability;
  std::vector<ToolbarPopoverItemView> items;
  std::vector<FieldView> fields;
};

using ToolbarItemView =
    std::variant<CommandView, ToolbarSegmentedView, ToolbarActionView,
                 ToolbarSeparatorView, ToolbarSpacerView, ToolbarPopoverView>;

struct ContextToolbarView {
  std::vector<ToolbarItemView> items;
};

struct ActivityView {
  Destination destination = Destination::Model;
  std::string label;
  std::string icon;
  Availability availability;
};

struct ExplorerView {
  std::string title;
  std::string search_placeholder = "Filter";
  std::vector<TreeRowView> rows;
  std::vector<CommandView> commands;
};

struct WorkspaceView {
  WorkspaceKind kind = WorkspaceKind::Model3d;
  std::string title;
  std::string empty_message;
  std::vector<ToolbarItemView> viewport_toolbar;
  SelectionTool model_selection_tool = SelectionTool::Pointer;
};

struct InspectorView {
  std::string title;
  std::string empty_message;
  std::vector<SectionView> sections;
  std::vector<CommandView> primary_commands;
};

struct ApplicationView {
  std::uint64_t revision = 0;
  ThemeMode theme_mode = ThemeMode::System;
  ApplicationBarView application_bar;
  ContextToolbarView context_toolbar;
  std::vector<ActivityView> activities;
  Destination active_destination = Destination::Model;
  ExplorerView explorer;
  WorkspaceView workspace;
  InspectorView inspector;
  std::optional<OperationView> operation;
  std::vector<StatusItemView> status_items;
};

[[nodiscard]] const CommandView *FindCommand(const ApplicationView &view,
                                             CommandId command);

} // namespace fancy_ui::steppenface
