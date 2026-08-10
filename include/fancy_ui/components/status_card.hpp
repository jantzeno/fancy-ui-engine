#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct StatusCardSpec {
  std::string_view id;
  std::string_view title;
  std::string_view message;
  SemanticStatus status = SemanticStatus::Information;
  IconPainter icon;
};

void StatusCard(const StatusCardSpec &spec);

} // namespace fancy_ui
