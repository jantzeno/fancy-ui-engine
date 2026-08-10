#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct OperationDisclosureSpec {
  std::string_view id;
  bool expanded = false;
  IconPainter icon;
  Availability availability;
};

struct OperationDisclosureResult : InteractionResult {
  bool changed = false;
  bool expanded = false;
};

[[nodiscard]] OperationDisclosureResult
OperationDisclosure(const OperationDisclosureSpec &spec);

} // namespace fancy_ui
