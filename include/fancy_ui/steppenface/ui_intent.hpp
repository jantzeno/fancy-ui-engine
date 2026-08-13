#pragma once

#include "fancy_ui/steppenface/ui_types.hpp"

#include <optional>
#include <variant>
#include <vector>

namespace fancy_ui::steppenface {

struct InvokeCommand {
  std::uint64_t revision = 0;
  UiId control;
  CommandId command = CommandId::Quit;
  std::optional<UiId> target;
};

struct ChangeSelection {
  std::uint64_t revision = 0;
  UiId source;
  UiId entity;
  SelectionMode mode = SelectionMode::Replace;
};

struct ChangeExpansion {
  std::uint64_t revision = 0;
  UiId source;
  UiId entity;
  bool expanded = false;
};

struct DropEntities {
  std::uint64_t revision = 0;
  UiId source_control;
  std::vector<UiId> entities;
  UiId target_control;
  UiId target;
};

struct EditField {
  std::uint64_t revision = 0;
  UiId field;
  FieldValue value;
  std::optional<UiId> target;
};

using UiIntent = std::variant<InvokeCommand, ChangeSelection, ChangeExpansion,
                              DropEntities, EditField>;

} // namespace fancy_ui::steppenface
