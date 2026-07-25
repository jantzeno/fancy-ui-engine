#pragma once

#include "fancy_ui/component_types.hpp"

#include <string_view>

namespace fancy_ui {

struct NotificationSpec {
  std::string_view id;
  std::string_view title;
  std::string_view message;
  SemanticStatus status = SemanticStatus::Information;
};

void Notification(const NotificationSpec &spec);

} // namespace fancy_ui
