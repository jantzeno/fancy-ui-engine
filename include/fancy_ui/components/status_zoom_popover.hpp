#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

inline constexpr float kStatusZoomMinimumPercent = 10.0f;
inline constexpr float kStatusZoomMaximumPercent = 1600.0f;

[[nodiscard]] float StatusZoomPercentFromSliderPosition(float position);
[[nodiscard]] float StatusZoomSliderPositionFromPercent(float percent);

enum class StatusZoomCommand {
  Fit,
  Selection,
  ActualSize,
};

struct StatusZoomPopoverState {
  bool open = false;
  bool restore_focus = false;
};

struct StatusZoomPopoverSpec {
  std::string_view id;
  float percent = 100.0f;
  bool request_open = false;
  Availability selection_availability;
};

struct StatusZoomPopoverResult : InteractionResult {
  bool opened = false;
  bool closed = false;
  bool changed = false;
  bool committed = false;
  bool popup_open = false;
  float percent = 100.0f;
  std::optional<StatusZoomCommand> command;
};

[[nodiscard]] StatusZoomPopoverResult
StatusZoomPopover(const StatusZoomPopoverSpec &spec,
                  StatusZoomPopoverState &state);

} // namespace fancy_ui
