#pragma once

#include <cstdint>

namespace fancy_ui {

enum class UiFontWeight : std::uint16_t {
  Thin = 100,
  ExtraLight = 200,
  Light = 300,
  Regular = 400,
  Medium = 500,
  SemiBold = 600,
  Bold = 700,
  ExtraBold = 800,
  Black = 900,
};

struct UiEnvironment {
  float base_font_em = 40.0f / 3.0f;
  UiFontWeight body_weight = UiFontWeight::Regular;
  float layout_scale = 1.0f;
  float raster_scale = 1.0f;
};

[[nodiscard]] UiEnvironment DetectDesktopUiEnvironment(float display_scale,
                                                       float pixel_density);

void UpdateDisplayScales(UiEnvironment &environment, float display_scale,
                         float pixel_density);

} // namespace fancy_ui
