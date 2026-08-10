#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

struct ProgressBarSpec {
  std::string_view id;
  std::string_view label;
  std::optional<float> value;
  SemanticStatus status = SemanticStatus::Busy;
  Vec2 size;
};

void ProgressBar(const ProgressBarSpec &spec);

} // namespace fancy_ui
