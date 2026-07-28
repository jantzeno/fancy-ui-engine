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
  std::string_view picker_title = "Choose color";
  ColorRgba value;
  std::span<const ColorRgba> colors;
  bool show_alpha = true;
  Availability availability;
};

/**
 * Retains one transactional color edit without hiding state in the component.
 *
 * Callers store this beside the edited value. The component owns only the
 * popup presentation: opening copies value into draft, Apply or Enter commits
 * it, and Cancel or Escape leaves the caller's value unchanged.
 */
struct ColorPickerState {
  bool editing = false;
  bool restore_focus = false;
  ColorRgba original;
  ColorRgba draft;
};

struct ColorPickerPopupSpec {
  std::string_view id;
  std::string_view title = "Choose color";
  ColorRgba value;
  bool request_open = false;
  bool show_alpha = true;
};

struct ColorPickerPopupResult {
  bool opened = false;
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  bool picker_open = false;
  ColorRgba value;
};

/**
 * Draws a transactional color-picker popup for an arbitrary trigger.
 *
 * This is shared by ColorSwatch and inline hierarchy color actions. The caller
 * supplies a stable ID and owns both the committed color and ColorPickerState.
 */
[[nodiscard]] ColorPickerPopupResult
ColorPickerPopup(const ColorPickerPopupSpec &spec, ColorPickerState &state);

struct ColorSwatchResult : InteractionResult {
  bool activated = false;
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  bool picker_open = false;
  ColorRgba value;
};

/**
 * Draws a color swatch button and its keyboard-accessible picker.
 *
 * When colors contains multiple values the button previews each value. A
 * committed edit returns one replacement color so callers can clear their
 * mixed state explicitly.
 */
[[nodiscard]] ColorSwatchResult ColorSwatch(const ColorSwatchSpec &spec,
                                            ColorPickerState &state);

} // namespace fancy_ui
