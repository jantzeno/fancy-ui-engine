#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct StatusTextSpec {
  std::string_view label;
  SemanticStatus status = SemanticStatus::Neutral;
};

void StatusText(const StatusTextSpec &spec);

} // namespace fancy_ui
