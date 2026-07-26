#pragma once

#include "fancy_ui/steppenface/ui_types.hpp"

#include <cstdint>
#include <string>
#include <variant>

namespace fancy_ui::steppenface {

enum class EditPhase : std::uint8_t {
  Changed,
  Commit,
  Cancel,
};

struct InvokeCommand {
  std::uint64_t revision = 0;
  CommandId command = CommandId::Quit;
};

struct ChangeSelection {
  std::uint64_t revision = 0;
  UiId entity;
  bool additive = false;
  bool range = false;
};

struct EditField {
  std::uint64_t revision = 0;
  UiId field;
  FieldValue value;
  EditPhase phase = EditPhase::Commit;
};

using UiIntent = std::variant<InvokeCommand, ChangeSelection, EditField>;

} // namespace fancy_ui::steppenface
