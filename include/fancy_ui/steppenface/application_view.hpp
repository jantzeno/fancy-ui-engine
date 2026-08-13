#pragma once

#include "fancy_ui/steppenface/ui_types.hpp"
#include "fancy_ui/theme.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace fancy_ui::steppenface {

struct ValidationView {
  bool invalid = false;
  std::string message;
};

struct EditBindingView {
  UiId field;
  std::optional<UiId> target;
};

struct ControlActionView {
  UiId field;
  FieldValue value;
  std::optional<UiId> target;
  Availability availability;
};

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
  bool selected = false;
  std::optional<CommandView> command;
  std::optional<ControlActionView> action;
  std::optional<WorkspaceKind> workspace;
  std::vector<MenuItemView> children;
};

struct ContextMenuView {
  UiId id;
  std::vector<MenuItemView> items;
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

struct ChoiceOptionView {
  UiId id;
  std::string label;
  std::string tooltip;
  Availability availability;
};

struct ToggleOptionView {
  ChoiceOptionView option;
  ToggleState state = ToggleState::Off;
};

struct TextFieldView {
  std::string value;
  std::string placeholder;
  std::size_t capacity = 512;
};

struct NumericFieldView {
  double value = 0.0;
  std::optional<double> minimum;
  std::optional<double> maximum;
  std::string unit;
  std::string format = "%.3f";
  bool integral = false;
};

struct SelectFieldView {
  std::vector<ChoiceOptionView> options;
  UiId selected;
};

struct RenamableSelectFieldView {
  std::vector<ChoiceOptionView> options;
  UiId selected;
  EditBindingView rename;
};

struct MultiselectFieldView {
  std::string summary;
  std::vector<ToggleOptionView> options;
};

struct SegmentedFieldView {
  std::vector<ChoiceOptionView> options;
  UiId selected;
  float width = 0.0f;
};

struct CheckboxFieldView {
  ToggleState state = ToggleState::Off;
  std::string value;
  std::string on_icon;
  std::string off_icon;
  bool show_checkbox = true;
};

struct VisibilityFieldView {
  ToggleState state = ToggleState::Off;
};

struct SliderFieldView {
  float value = 0.0f;
  float minimum = 0.0f;
  float maximum = 1.0f;
  std::string unit;
  std::string format = "%.3f";
};

struct RotationCompassFieldView {
  int count = 4;
  bool inherited = false;
};

struct DurationFieldView {
  DurationValue value;
};

struct ColorFieldView {
  ColorRgba value;
  std::vector<ColorRgba> colors;
  bool show_alpha = true;
};

struct ValueFieldView {
  std::string value;
  bool mixed = false;
};

struct ButtonFieldView {
  CommandView command;
};

using FieldContent =
    std::variant<TextFieldView, NumericFieldView, SelectFieldView,
                 RenamableSelectFieldView, MultiselectFieldView,
                 SegmentedFieldView, CheckboxFieldView, VisibilityFieldView,
                 SliderFieldView, RotationCompassFieldView, DurationFieldView,
                 ColorFieldView, ValueFieldView, ButtonFieldView>;

enum class FieldLabelLayout : std::uint8_t {
  Inline,
  Stacked,
};

[[nodiscard]] FieldLabelLayout FieldLabelLayoutFor(const FieldContent &content);

struct FieldView {
  UiId id;
  std::string label;
  std::string tooltip;
  std::string help;
  Availability availability;
  ValidationView validation;
  std::optional<EditBindingView> edit;
  FieldContent content;
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

struct ExplorerSearchView {
  UiId id{.value = "explorer.search"};
  std::string placeholder = "Filter";
  Availability availability;
};

struct HierarchyRowView {
  UiId id;
  UiId entity;
  std::string label;
  std::string secondary_label;
  std::string icon;
  int depth = 0;
  bool expanded = false;
  bool expandable = false;
  bool selected = false;
  SemanticTone tone = SemanticTone::Neutral;
  Availability availability;
  std::optional<ColorRgba> color;
  std::optional<EditBindingView> color_edit;
  std::optional<ToggleState> visibility;
  std::optional<EditBindingView> visibility_edit;
  std::optional<ContextMenuView> context_menu;
};

struct ExplorerView {
  std::string title;
  std::string tree_label;
  std::string footer;
  ExplorerSearchView search;
  std::vector<HierarchyRowView> rows;
  std::vector<CommandView> commands;
};

struct WorkspaceView {
  WorkspaceKind kind = WorkspaceKind::Model3d;
  UiId selection_source{.value = "workspace.model"};
  Availability selection_availability;
  std::string title;
  std::string empty_message;
  std::vector<ToolbarItemView> viewport_toolbar;
  SelectionTool model_selection_tool = SelectionTool::Pointer;
};

struct MetricValueView {
  std::string label;
  std::string value;
  bool wide = false;
  bool stacked = false;
};

struct InformationTreeRowView {
  UiId id;
  UiId entity;
  std::string label;
  std::string metadata;
  int depth = 0;
  bool expanded = false;
  bool expandable = false;
  bool selected = false;
  bool highlighted = false;
  SemanticTone tone = SemanticTone::Neutral;
  Availability availability;
  std::vector<MetricValueView> metrics;
  std::optional<ToggleState> visibility;
  std::optional<EditBindingView> visibility_edit;
  std::vector<CommandView> actions;
};

struct StatusCardView {
  UiId id;
  std::string title;
  std::string message;
  SemanticTone tone = SemanticTone::Information;
  std::string icon;
};

struct SectionView {
  UiId id;
  std::string heading;
  std::string summary;
  bool default_open = true;
  bool focused = false;
  std::optional<CommandView> header_command;
  std::vector<InformationTreeRowView> information_rows;
  std::vector<FieldView> fields;
  std::vector<CommandView> actions;
  std::optional<StatusCardView> status;
};

enum class InspectorHeaderMode : std::uint8_t {
  Selection,
  Task,
};

struct InspectorView {
  InspectorHeaderMode header_mode = InspectorHeaderMode::Selection;
  std::string title;
  std::string subtitle;
  std::string scope;
  std::string note;
  std::string empty_message;
  std::vector<SectionView> sections;
  std::optional<CommandView> primary_command;
  std::vector<CommandView> secondary_commands;
};

/**
 * Typed native shape used to translate a canonical panel-audit PanelState.
 *
 * The product bridge supplies one immutable contract per frame. Explorer and
 * Inspector stay together so review fixtures and production use the same
 * presentation path.
 */
struct PanelContractView {
  UiId id;
  std::string label;
  Destination destination = Destination::Model;
  ExplorerView explorer;
  InspectorView inspector;
};

struct OperationView {
  UiId id;
  std::string title;
  std::string summary;
  SemanticTone tone = SemanticTone::Neutral;
  float progress = 0.0f;
  bool indeterminate = false;
  std::vector<CommandView> commands;
};

struct StatusItemView {
  UiId id;
  std::string label;
  SemanticTone tone = SemanticTone::Neutral;
};

struct SystemSettingsView {
  bool open = false;
  std::string title = "System Settings";
  std::string description;
  FieldView theme;
  ResolvedTheme preview_theme = ResolvedTheme::Dark;
  bool dirty = false;
  CommandView apply;
  CommandView discard;
  CommandView close;
};

struct ApplicationView {
  std::uint64_t revision = 0;
  ThemeMode theme_mode = ThemeMode::System;
  std::optional<ResolvedTheme> system_theme;
  std::optional<SystemSettingsView> settings;
  ApplicationBarView application_bar;
  ContextToolbarView context_toolbar;
  std::vector<ActivityView> activities;
  PanelContractView panel;
  WorkspaceView workspace;
  std::optional<OperationView> operation;
  std::vector<StatusItemView> status_items;
};

[[nodiscard]] const CommandView *FindCommand(const ApplicationView &view,
                                             CommandId command);
[[nodiscard]] const CommandView *FindCommand(const ApplicationView &view,
                                             const UiId &control,
                                             CommandId command);
/** Finds an exact command binding, including its stable product target. */
[[nodiscard]] const CommandView *FindCommand(const ApplicationView &view,
                                             const UiId &control,
                                             CommandId command,
                                             const std::optional<UiId> &target);
[[nodiscard]] const Availability *
FindEditBinding(const ApplicationView &view, const UiId &field,
                const std::optional<UiId> &target, const FieldValue &value);
[[nodiscard]] const Availability *FindSelectable(const ApplicationView &view,
                                                 const UiId &source,
                                                 const UiId &entity);

} // namespace fancy_ui::steppenface
