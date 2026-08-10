#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

enum class ResizeDirection {
  Horizontal,
  Vertical,
};

enum class ResizeCommand {
  Decrease,
  Increase,
  Minimum,
  Maximum,
};

[[nodiscard]] float ClampResizeValue(float value, float minimum, float maximum);
[[nodiscard]] float ResizeValueAfterCommand(float value, float minimum,
                                            float maximum, float step,
                                            ResizeCommand command);

struct ResizeHandleSpec {
  std::string_view id;
  float value = 0.0f;
  float minimum = 0.0f;
  float maximum = 0.0f;
  float keyboard_step = 8.0f;
  std::optional<float> reset_value;
  ResizeDirection direction = ResizeDirection::Vertical;
  std::string_view tooltip;
};

struct ResizeHandleResult : InteractionResult {
  bool changed = false;
  float value = 0.0f;
};

[[nodiscard]] ResizeHandleResult ResizeHandle(const ResizeHandleSpec &spec);

} // namespace fancy_ui
