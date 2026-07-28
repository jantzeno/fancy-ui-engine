#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

/**
 * Presents a compact semantic fact with a reinforcing host-provided icon.
 */
struct StatusCardSpec {
  std::string_view id;
  std::string_view title;
  std::string_view message;
  SemanticStatus status = SemanticStatus::Information;
  IconPainter icon;
};

void StatusCard(const StatusCardSpec &spec);

struct NotificationSpec {
  std::string_view id;
  std::string_view title;
  std::string_view message;
  SemanticStatus status = SemanticStatus::Information;
  IconPainter icon;
};

void Notification(const NotificationSpec &spec);

/**
 * Draws determinate progress in the closed interval [0, 1].
 *
 * An absent value renders the static indeterminate state used when the caller
 * can report activity but cannot calculate completion.
 */
struct ProgressBarSpec {
  std::string_view id;
  std::string_view label;
  std::optional<float> value;
  SemanticStatus status = SemanticStatus::Busy;
  Vec2 size = {.x = 0.0f, .y = 6.0f};
};

void ProgressBar(const ProgressBarSpec &spec);

} // namespace fancy_ui
