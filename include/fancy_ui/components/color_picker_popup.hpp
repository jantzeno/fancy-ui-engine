#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/components/color_picker_types.hpp"

#include <string_view>

namespace fancy_ui {

struct ColorPickerPopupSpec {
  std::string_view id;
  std::string_view title = "Choose color";
  ColorRgba value;
  bool request_open = false;
  bool show_alpha = true;
  ColorPickerLayout layout = ColorPickerLayout::CurrentAndOriginal;
};

struct ColorPickerPopupResult {
  bool opened = false;
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  bool picker_open = false;
  ColorRgba value;
};

[[nodiscard]] ColorPickerPopupResult
ColorPickerPopup(const ColorPickerPopupSpec &spec, ColorPickerState &state);

} // namespace fancy_ui
