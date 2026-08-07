#pragma once

#include <cstdint>
#include <functional>
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

struct FontHandle {
  std::uintptr_t value = 0;

  [[nodiscard]] explicit operator bool() const { return value != 0; }
  [[nodiscard]] bool operator==(const FontHandle &) const = default;
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
 * Describes a persistent validation cue without moving validation rules into
 * the drawing layer.
 *
 * The caller owns the rule and recovery message. Components keep this cue
 * independent from selection and keyboard focus.
 */
struct Validation {
  bool invalid = false;
  std::string_view message;
};

enum class ToggleState {
  Off,
  On,
  Mixed,
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
  Busy,
  Preview,
};

/**
 * Draws a monochrome icon without exposing a renderer type in public headers.
 *
 * Components provide logical bounds and a semantic foreground color. The host
 * decides how those values map to its icon atlas.
 */
using IconPainter = std::function<void(const Rect &, ColorRgba)>;

} // namespace fancy_ui
