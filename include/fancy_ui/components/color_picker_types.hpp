#pragma once

#include "fancy_ui/component_types.hpp"

namespace fancy_ui {

enum class ColorPickerLayout {
  CurrentAndOriginal,
  Compact,
};

struct ColorPickerState {
  bool editing = false;
  bool restore_focus = false;
  ColorRgba original;
  ColorRgba draft;
};

} // namespace fancy_ui
