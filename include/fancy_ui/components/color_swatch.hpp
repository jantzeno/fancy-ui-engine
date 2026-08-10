#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/color_picker_types.hpp"

#include <span>
#include <string_view>

namespace fancy_ui {

struct ColorSwatchSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  std::string_view picker_title = "Choose color";
  ColorRgba value;
  std::span<const ColorRgba> colors;
  bool show_alpha = true;
  ColorPickerLayout picker_layout = ColorPickerLayout::CurrentAndOriginal;
  Availability availability;
};

struct ColorSwatchResult : InteractionResult {
  bool activated = false;
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  bool picker_open = false;
  ColorRgba value;
};

[[nodiscard]] ColorSwatchResult ColorSwatch(const ColorSwatchSpec &spec,
                                            ColorPickerState &state);

} // namespace fancy_ui
