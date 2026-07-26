#pragma once

#include <cstdint>
#include <string_view>

namespace fancy_ui {

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
};

struct Rect {
  Vec2 minimum;
  Vec2 maximum;
};

struct ColorRgba {
  float red = 0.0f;
  float green = 0.0f;
  float blue = 0.0f;
  float alpha = 1.0f;

  [[nodiscard]] bool operator==(const ColorRgba &) const = default;
};

struct TextureHandle {
  std::uintptr_t value = 0;

  [[nodiscard]] explicit operator bool() const { return value != 0; }
  [[nodiscard]] bool operator==(const TextureHandle &) const = default;
};

/**
 * Describes whether a prepared command can be invoked by a component.
 *
 * The caller calculates availability and its explanation from product state.
 * Components only render the supplied state.
 */
struct Availability {
  bool enabled = true;
  bool busy = false;
  std::string_view reason;
};

/**
 * Interaction facts common to all immediate-mode components.
 */
struct InteractionResult {
  bool hovered = false;
  bool focused = false;
  bool active = false;
};

enum class SemanticStatus {
  Neutral,
  Information,
  Success,
  Warning,
  Failure,
};

} // namespace fancy_ui
