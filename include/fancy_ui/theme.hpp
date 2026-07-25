#pragma once

#include <imgui.h>

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
  ImVec4 application_surface;
  ImVec4 canvas;
  ImVec4 surface;
  ImVec4 surface_raised;
  ImVec4 control;
  ImVec4 control_hover;
  ImVec4 control_pressed;
  ImVec4 border;
  ImVec4 border_strong;
  ImVec4 text_primary;
  ImVec4 text_secondary;
  ImVec4 text_disabled;
  ImVec4 focus;
  ImVec4 selection;
  ImVec4 action_primary;
  ImVec4 action_primary_hover;
  ImVec4 action_primary_pressed;
  ImVec4 on_emphasis;
  ImVec4 information;
  ImVec4 information_background;
  ImVec4 success;
  ImVec4 success_background;
  ImVec4 warning;
  ImVec4 warning_background;
  ImVec4 failure;
  ImVec4 failure_background;
};

[[nodiscard]] ResolvedTheme
ResolveTheme(ThemeMode mode,
             std::optional<ResolvedTheme> system_theme = std::nullopt);
[[nodiscard]] SemanticPalette PaletteFor(ResolvedTheme theme);

/**
 * Applies a resolved semantic palette to Dear ImGui and stores it for shared
 * components drawn during subsequent frames.
 */
void ApplyTheme(ResolvedTheme theme);

[[nodiscard]] const SemanticPalette &CurrentPalette();

} // namespace fancy_ui
