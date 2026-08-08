#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/ui_environment.hpp"
#include <optional>

namespace fancy_ui {

enum class ThemeMode {
  System,
  Light,
  Dark,
};

enum class ResolvedTheme {
  Light,
  Dark,
};

/**
 * Semantic colors shared by controls and shell composition.
 */
struct SemanticPalette {
  ColorRgba application_surface;
  ColorRgba canvas;
  ColorRgba surface;
  ColorRgba surface_muted;
  ColorRgba surface_raised;
  ColorRgba control;
  ColorRgba control_hover;
  ColorRgba control_pressed;
  ColorRgba control_disabled_fill;
  ColorRgba control_disabled_border;
  ColorRgba border;
  ColorRgba border_strong;
  ColorRgba text_primary;
  ColorRgba text_secondary;
  ColorRgba text_disabled;
  ColorRgba focus;
  ColorRgba selection;
  ColorRgba action_primary;
  ColorRgba action_primary_hover;
  ColorRgba action_primary_pressed;
  ColorRgba on_emphasis;
  ColorRgba information;
  ColorRgba information_background;
  ColorRgba success;
  ColorRgba success_background;
  ColorRgba warning;
  ColorRgba warning_background;
  ColorRgba failure;
  ColorRgba failure_background;
};

[[nodiscard]] ResolvedTheme
ResolveTheme(ThemeMode mode,
             std::optional<ResolvedTheme> system_theme = std::nullopt);
[[nodiscard]] SemanticPalette PaletteFor(ResolvedTheme theme);

/**
 * Applies a resolved semantic palette to Dear ImGui and stores it for shared
 * components drawn during subsequent frames.
 */
void ApplyTheme(ResolvedTheme theme,
                const UiEnvironment &environment = UiEnvironment{});

[[nodiscard]] const SemanticPalette &CurrentPalette();
[[nodiscard]] const UiEnvironment &CurrentUiEnvironment();
[[nodiscard]] float CurrentUiScale();
[[nodiscard]] float Scale(float logical_pixels);

} // namespace fancy_ui
