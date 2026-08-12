#include "fancy_ui/steppenface/application_view.hpp"

#include <type_traits>
#include <utility>

namespace fancy_ui::steppenface {

namespace {

template <typename Predicate>
const CommandView *FindMenuCommand(const std::vector<MenuItemView> &items,
                                   Predicate &&matches) {
  for (const MenuItemView &item : items) {
    if (item.command.has_value() && matches(*item.command)) {
      return &*item.command;
    }
    if (const CommandView *nested =
            FindMenuCommand(item.children, std::forward<Predicate>(matches));
        nested != nullptr) {
      return nested;
    }
  }
  return nullptr;
}

template <typename Predicate>
const CommandView *FindCommand(const std::vector<CommandView> &commands,
                               Predicate &&matches) {
  for (const CommandView &candidate : commands) {
    if (matches(candidate)) {
      return &candidate;
    }
  }
  return nullptr;
}

template <typename Predicate>
const CommandView *FindToolbarCommand(const std::vector<ToolbarItemView> &items,
                                      Predicate &&matches) {
  for (const ToolbarItemView &item : items) {
    if (const CommandView *candidate = std::get_if<CommandView>(&item);
        candidate != nullptr && matches(*candidate)) {
      return candidate;
    }
    const ToolbarPopoverView *popover = std::get_if<ToolbarPopoverView>(&item);
    if (popover == nullptr) {
      continue;
    }
    for (const ToolbarPopoverItemView &popover_item : popover->items) {
      if (const CommandView *candidate =
              std::get_if<CommandView>(&popover_item);
          candidate != nullptr && matches(*candidate)) {
        return candidate;
      }
    }
  }
  return nullptr;
}

template <typename Predicate>
const CommandView *FindAnyCommand(const ApplicationView &view,
                                  Predicate &&matches) {
  for (const ApplicationMenuView &menu : view.application_bar.menus) {
    if (const CommandView *found =
            FindMenuCommand(menu.items, std::forward<Predicate>(matches));
        found != nullptr) {
      return found;
    }
  }
  if (const CommandView *found = FindToolbarCommand(
          view.context_toolbar.items, std::forward<Predicate>(matches));
      found != nullptr) {
    return found;
  }
  if (const CommandView *found = FindToolbarCommand(
          view.workspace.viewport_toolbar, std::forward<Predicate>(matches));
      found != nullptr) {
    return found;
  }
  if (const CommandView *found = FindCommand(view.panel.explorer.commands,
                                             std::forward<Predicate>(matches));
      found != nullptr) {
    return found;
  }
  for (const HierarchyRowView &row : view.panel.explorer.rows) {
    if (!row.context_menu.has_value()) {
      continue;
    }
    if (const CommandView *found = FindMenuCommand(
            row.context_menu->items, std::forward<Predicate>(matches));
        found != nullptr) {
      return found;
    }
  }
  if (view.panel.inspector.primary_command.has_value() &&
      matches(*view.panel.inspector.primary_command)) {
    return &*view.panel.inspector.primary_command;
  }
  if (const CommandView *found =
          FindCommand(view.panel.inspector.secondary_commands,
                      std::forward<Predicate>(matches));
      found != nullptr) {
    return found;
  }
  for (const SectionView &section : view.panel.inspector.sections) {
    if (section.header_command.has_value() &&
        matches(*section.header_command)) {
      return &*section.header_command;
    }
    if (const CommandView *found =
            FindCommand(section.actions, std::forward<Predicate>(matches));
        found != nullptr) {
      return found;
    }
    for (const FieldView &field : section.fields) {
      const ButtonFieldView *button =
          std::get_if<ButtonFieldView>(&field.content);
      if (button != nullptr && matches(button->command)) {
        return &button->command;
      }
    }
    for (const InformationTreeRowView &row : section.information_rows) {
      if (const CommandView *found =
              FindCommand(row.actions, std::forward<Predicate>(matches));
          found != nullptr) {
        return found;
      }
    }
  }
  if (view.operation.has_value()) {
    return FindCommand(view.operation->commands,
                       std::forward<Predicate>(matches));
  }
  return nullptr;
}

bool SameBinding(const UiId &field, const std::optional<UiId> &target,
                 const UiId &candidate_field,
                 const std::optional<UiId> &candidate_target) {
  return field == candidate_field && target == candidate_target;
}

const Availability *FindMenuEdit(const std::vector<MenuItemView> &items,
                                 const UiId &field,
                                 const std::optional<UiId> &target,
                                 const FieldValue &value) {
  for (const MenuItemView &item : items) {
    if (item.action.has_value() &&
        SameBinding(field, target, item.action->field, item.action->target) &&
        item.action->value == value) {
      return &item.action->availability;
    }
    if (const Availability *nested =
            FindMenuEdit(item.children, field, target, value);
        nested != nullptr) {
      return nested;
    }
  }
  return nullptr;
}

const Availability *FindToolbarEdit(const std::vector<ToolbarItemView> &items,
                                    const UiId &field,
                                    const std::optional<UiId> &target,
                                    const FieldValue &value);

bool Prepared(const Availability &availability) {
  return availability.visible && availability.enabled && !availability.busy;
}

const Availability *FindOption(const FieldView &field,
                               const std::vector<ChoiceOptionView> &options,
                               const UiId &id) {
  if (!Prepared(field.availability)) {
    return &field.availability;
  }
  for (const ChoiceOptionView &option : options) {
    if (option.id == id) {
      return &option.availability;
    }
  }
  return nullptr;
}

const Availability *FieldEditAvailability(const FieldView &field,
                                          const FieldValue &value) {
  return std::visit(
      [&field, &value](const auto &content) -> const Availability * {
        using Content = std::decay_t<decltype(content)>;
        if constexpr (std::is_same_v<Content, TextFieldView>) {
          return std::holds_alternative<std::string>(value)
                     ? &field.availability
                     : nullptr;
        } else if constexpr (std::is_same_v<Content, NumericFieldView>) {
          const bool typed = content.integral
                                 ? std::holds_alternative<std::int64_t>(value)
                                 : std::holds_alternative<double>(value);
          return typed ? &field.availability : nullptr;
        } else if constexpr (std::is_same_v<Content, SelectFieldView> ||
                             std::is_same_v<Content,
                                            RenamableSelectFieldView> ||
                             std::is_same_v<Content, SegmentedFieldView>) {
          const UiId *selected = std::get_if<UiId>(&value);
          return selected == nullptr
                     ? nullptr
                     : FindOption(field, content.options, *selected);
        } else if constexpr (std::is_same_v<Content, MultiselectFieldView>) {
          const ChoiceToggleValue *selected =
              std::get_if<ChoiceToggleValue>(&value);
          if (selected == nullptr) {
            return nullptr;
          }
          if (!Prepared(field.availability)) {
            return &field.availability;
          }
          for (const ToggleOptionView &option : content.options) {
            if (option.option.id == selected->option) {
              return &option.option.availability;
            }
          }
          return nullptr;
        } else if constexpr (std::is_same_v<Content, CheckboxFieldView> ||
                             std::is_same_v<Content, VisibilityFieldView>) {
          return std::holds_alternative<ToggleState>(value)
                     ? &field.availability
                     : nullptr;
        } else if constexpr (std::is_same_v<Content, SliderFieldView>) {
          return std::holds_alternative<double>(value) ? &field.availability
                                                       : nullptr;
        } else if constexpr (std::is_same_v<Content,
                                            RotationCompassFieldView>) {
          return std::holds_alternative<std::int64_t>(value)
                     ? &field.availability
                     : nullptr;
        } else if constexpr (std::is_same_v<Content, DurationFieldView>) {
          return std::holds_alternative<DurationValue>(value)
                     ? &field.availability
                     : nullptr;
        } else if constexpr (std::is_same_v<Content, ColorFieldView>) {
          return std::holds_alternative<ColorRgba>(value) ? &field.availability
                                                          : nullptr;
        }
        return nullptr;
      },
      field.content);
}

const Availability *FindToolbarEdit(const std::vector<ToolbarItemView> &items,
                                    const UiId &field,
                                    const std::optional<UiId> &target,
                                    const FieldValue &value) {
  for (const ToolbarItemView &item : items) {
    if (const ToolbarActionView *action = std::get_if<ToolbarActionView>(&item);
        action != nullptr &&
        SameBinding(field, target, action->action.field,
                    action->action.target) &&
        action->action.value == value) {
      return &action->action.availability;
    }
    if (const ToolbarSegmentedView *segmented =
            std::get_if<ToolbarSegmentedView>(&item);
        segmented != nullptr) {
      for (const ToolbarChoiceView &choice : segmented->choices) {
        if (SameBinding(field, target, choice.action.field,
                        choice.action.target) &&
            choice.action.value == value) {
          return &choice.action.availability;
        }
      }
    }
    const ToolbarPopoverView *popover = std::get_if<ToolbarPopoverView>(&item);
    if (popover == nullptr) {
      continue;
    }
    for (const ToolbarPopoverItemView &popover_item : popover->items) {
      const ToolbarMenuItemView *menu_action =
          std::get_if<ToolbarMenuItemView>(&popover_item);
      if (menu_action != nullptr &&
          SameBinding(field, target, menu_action->action.field,
                      menu_action->action.target) &&
          menu_action->action.value == value) {
        return &menu_action->action.availability;
      }
    }
    for (const FieldView &candidate : popover->fields) {
      if (candidate.edit.has_value() &&
          SameBinding(field, target, candidate.edit->field,
                      candidate.edit->target)) {
        return FieldEditAvailability(candidate, value);
      }
    }
  }
  return nullptr;
}

} // namespace

FieldLabelLayout FieldLabelLayoutFor(const FieldContent &content) {
  return std::visit(
      [](const auto &value) {
        using Content = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Content, SelectFieldView> ||
                      std::is_same_v<Content, RenamableSelectFieldView> ||
                      std::is_same_v<Content, MultiselectFieldView> ||
                      std::is_same_v<Content, SliderFieldView> ||
                      std::is_same_v<Content, RotationCompassFieldView>) {
          return FieldLabelLayout::Stacked;
        }
        return FieldLabelLayout::Inline;
      },
      content);
}

const CommandView *FindMenuCommand(const ApplicationBarView &application_bar,
                                   const CommandId command) {
  for (const ApplicationMenuView &menu : application_bar.menus) {
    if (const CommandView *found =
            FindMenuCommand(menu.items,
                            [command](const CommandView &candidate) {
                              return candidate.command == command;
                            });
        found != nullptr) {
      return found;
    }
  }
  return nullptr;
}

const CommandView *FindCommand(const ApplicationView &view,
                               const CommandId command) {
  return FindAnyCommand(view, [command](const CommandView &candidate) {
    return candidate.command == command;
  });
}

const CommandView *FindCommand(const ApplicationView &view, const UiId &control,
                               const CommandId command) {
  return FindAnyCommand(
      view, [&control, command](const CommandView &candidate) {
        return candidate.id == control && candidate.command == command;
      });
}

const Availability *FindEditBinding(const ApplicationView &view,
                                    const UiId &field,
                                    const std::optional<UiId> &target,
                                    const FieldValue &value) {
  for (const ApplicationMenuView &menu : view.application_bar.menus) {
    if (const Availability *found =
            FindMenuEdit(menu.items, field, target, value);
        found != nullptr) {
      return found;
    }
  }
  if (const Availability *found =
          FindToolbarEdit(view.context_toolbar.items, field, target, value);
      found != nullptr) {
    return found;
  }
  if (const Availability *found = FindToolbarEdit(
          view.workspace.viewport_toolbar, field, target, value);
      found != nullptr) {
    return found;
  }
  for (const HierarchyRowView &row : view.panel.explorer.rows) {
    if (row.color_edit.has_value() &&
        SameBinding(field, target, row.color_edit->field,
                    row.color_edit->target) &&
        std::holds_alternative<ColorRgba>(value)) {
      return &row.availability;
    }
    if (row.visibility_edit.has_value() &&
        SameBinding(field, target, row.visibility_edit->field,
                    row.visibility_edit->target) &&
        std::holds_alternative<ToggleState>(value)) {
      return &row.availability;
    }
    if (row.context_menu.has_value()) {
      if (const Availability *found =
              FindMenuEdit(row.context_menu->items, field, target, value);
          found != nullptr) {
        return found;
      }
    }
  }
  for (const SectionView &section : view.panel.inspector.sections) {
    for (const FieldView &candidate : section.fields) {
      if (candidate.edit.has_value() &&
          SameBinding(field, target, candidate.edit->field,
                      candidate.edit->target)) {
        return FieldEditAvailability(candidate, value);
      }
      const RenamableSelectFieldView *renamable =
          std::get_if<RenamableSelectFieldView>(&candidate.content);
      if (renamable != nullptr &&
          SameBinding(field, target, renamable->rename.field,
                      renamable->rename.target) &&
          std::holds_alternative<std::string>(value)) {
        return &candidate.availability;
      }
    }
    for (const InformationTreeRowView &row : section.information_rows) {
      if (row.visibility_edit.has_value() &&
          SameBinding(field, target, row.visibility_edit->field,
                      row.visibility_edit->target) &&
          std::holds_alternative<ToggleState>(value)) {
        return &row.availability;
      }
    }
  }
  return nullptr;
}

const Availability *FindSelectable(const ApplicationView &view,
                                   const UiId &source, const UiId &entity) {
  for (const HierarchyRowView &row : view.panel.explorer.rows) {
    if (row.id == source && row.entity == entity) {
      return &row.availability;
    }
  }
  for (const SectionView &section : view.panel.inspector.sections) {
    for (const InformationTreeRowView &row : section.information_rows) {
      if (row.id == source && row.entity == entity) {
        return &row.availability;
      }
    }
  }
  if (view.workspace.selection_source == source && !entity.empty()) {
    return &view.workspace.selection_availability;
  }
  return nullptr;
}

} // namespace fancy_ui::steppenface
