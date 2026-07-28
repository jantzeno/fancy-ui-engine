#pragma once

#include "fancy_ui/component_types.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace fancy_ui {

struct NumericInputSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::string_view unit;
  double value = 0.0;
  std::optional<double> minimum;
  std::optional<double> maximum;
  std::string_view format = "%.3f";
  Availability availability;
  Validation validation;
};

struct NumericInputResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  double value = 0.0;
};

[[nodiscard]] NumericInputResult NumericInput(const NumericInputSpec &spec);

struct TextInputSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::string_view value;
  std::size_t capacity = 512;
  Availability availability;
  Validation validation;
};

struct TextInputResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  std::string value;
};

[[nodiscard]] TextInputResult TextInput(const TextInputSpec &spec);

struct SelectOption {
  std::string_view id;
  std::string_view label;
  bool enabled = true;
};

struct SelectSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::span<const SelectOption> options;
  std::size_t selected_index = 0;
  Availability availability;
  Validation validation;
};

struct SelectResult : InteractionResult {
  bool changed = false;
  std::size_t selected_index = 0;
};

[[nodiscard]] SelectResult Select(const SelectSpec &spec);

struct DurationSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  int hours = 0;
  int minutes = 0;
  Availability availability;
  Validation validation;
};

struct DurationResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  int hours = 0;
  int minutes = 0;
};

/**
 * Draws the shared paired duration editor.
 *
 * Hours stay within 0–23 and minutes within 0–59 so every caller uses the
 * same visible range and commit behavior.
 */
[[nodiscard]] DurationResult Duration(const DurationSpec &spec);

struct VisibilityToggleSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  ToggleState state = ToggleState::Off;
  IconPainter visible_icon;
  IconPainter hidden_icon;
  Availability availability;
};

struct VisibilityToggleResult : InteractionResult {
  bool changed = false;
  ToggleState state = ToggleState::Off;
};

[[nodiscard]] VisibilityToggleResult
VisibilityToggle(const VisibilityToggleSpec &spec);

struct ColorSwatchSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::span<const ColorRgba> colors;
  Availability availability;
};

struct ColorSwatchResult : InteractionResult {
  bool activated = false;
};

[[nodiscard]] ColorSwatchResult ColorSwatch(const ColorSwatchSpec &spec);

} // namespace fancy_ui
