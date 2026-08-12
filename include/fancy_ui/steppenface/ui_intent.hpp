#pragma once

#include "fancy_ui/steppenface/ui_types.hpp"

#include <optional>
#include <variant>

namespace fancy_ui::steppenface {

struct InvokeCommand {
  std::uint64_t revision = 0;
  UiId control;
  CommandId command = CommandId::Quit;
};

struct ChangeSelection {
  std::uint64_t revision = 0;
  UiId source;
  UiId entity;
  bool additive = false;
  bool range = false;
};

struct EditField {
  std::uint64_t revision = 0;
  UiId field;
  FieldValue value;
  std::optional<UiId> target;
};

using UiIntent = std::variant<InvokeCommand, ChangeSelection, EditField>;

} // namespace fancy_ui::steppenface
