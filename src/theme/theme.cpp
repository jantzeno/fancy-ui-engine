#include "fancy_ui/theme.hpp"

#include "fancy_ui/layout_metrics.hpp"

#include <imgui.h>

#include <algorithm>

namespace fancy_ui {

namespace {

ColorRgba Rgb(const int red, const int green, const int blue) {
  constexpr float scale = 1.0f / 255.0f;
  return ColorRgba{static_cast<float>(red) * scale,
                   static_cast<float>(green) * scale,
                   static_cast<float>(blue) * scale, 1.0f};
}

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

SemanticPalette active_palette = PaletteFor(ResolvedTheme::Dark);
float active_ui_scale = 1.0f;

} // namespace

ResolvedTheme ResolveTheme(const ThemeMode mode,
                           const std::optional<ResolvedTheme> system_theme) {
  switch (mode) {
  case ThemeMode::Light:
    return ResolvedTheme::Light;
  case ThemeMode::Dark:
    return ResolvedTheme::Dark;
  case ThemeMode::System:
    return system_theme.value_or(ResolvedTheme::Dark);
  }
  return ResolvedTheme::Dark;
}

SemanticPalette PaletteFor(const ResolvedTheme theme) {
  if (theme == ResolvedTheme::Light) {
    return {
        .application_surface = Rgb(0xF6, 0xF8, 0xFA),
        .canvas = Rgb(0xFF, 0xFF, 0xFF),
        .surface = Rgb(0xFF, 0xFF, 0xFF),
        .surface_muted = Rgb(0xF6, 0xF8, 0xFA),
        .surface_raised = Rgb(0xFF, 0xFF, 0xFF),
        .control = Rgb(0xFF, 0xFF, 0xFF),
        .control_hover = Rgb(0xEF, 0xF2, 0xF5),
        .control_pressed = Rgb(0xE6, 0xEA, 0xEF),
        .control_disabled_fill = Rgb(0xF6, 0xF8, 0xFA),
        .control_disabled_border = Rgb(0xD1, 0xD9, 0xE0),
        .border = Rgb(0xD1, 0xD9, 0xE0),
        .border_strong = Rgb(0x81, 0x8B, 0x98),
        .text_primary = Rgb(0x1F, 0x23, 0x28),
        .text_secondary = Rgb(0x59, 0x63, 0x6E),
        .text_disabled = Rgb(0x81, 0x8B, 0x98),
        .focus = Rgb(0x09, 0x69, 0xDA),
        .selection = Rgb(0xDD, 0xF4, 0xFF),
        .action_primary = Rgb(0x09, 0x69, 0xDA),
        .action_primary_hover = Rgb(0x08, 0x60, 0xCA),
        .action_primary_pressed = Rgb(0x07, 0x57, 0xBA),
        .on_emphasis = Rgb(0xFF, 0xFF, 0xFF),
        .information = Rgb(0x09, 0x69, 0xDA),
        .information_background = Rgb(0xDD, 0xF4, 0xFF),
        .success = Rgb(0x1A, 0x7F, 0x37),
        .success_background = Rgb(0xDA, 0xFB, 0xE1),
        .warning = Rgb(0x9A, 0x67, 0x00),
        .warning_background = Rgb(0xFF, 0xF8, 0xC5),
        .failure = Rgb(0xD1, 0x24, 0x2F),
        .failure_background = Rgb(0xFF, 0xEB, 0xE9),
    };
  }

  return {
      .application_surface = Rgb(0x01, 0x04, 0x09),
      .canvas = Rgb(0x0D, 0x11, 0x17),
      .surface = Rgb(0x0D, 0x11, 0x17),
      .surface_muted = Rgb(0x15, 0x1B, 0x23),
      .surface_raised = Rgb(0x21, 0x28, 0x30),
      .control = Rgb(0x21, 0x28, 0x30),
      .control_hover = Rgb(0x26, 0x2F, 0x3A),
      .control_pressed = Rgb(0x2D, 0x37, 0x43),
      .control_disabled_fill = Rgb(0x15, 0x1B, 0x23),
      .control_disabled_border = Rgb(0x3D, 0x44, 0x4D),
      .border = Rgb(0x3D, 0x44, 0x4D),
      .border_strong = Rgb(0x6E, 0x76, 0x81),
      .text_primary = Rgb(0xF0, 0xF6, 0xFC),
      .text_secondary = Rgb(0x91, 0x98, 0xA1),
      .text_disabled = Rgb(0x65, 0x6C, 0x76),
      .focus = Rgb(0x44, 0x93, 0xF8),
      .selection = Rgb(0x18, 0x2B, 0x44),
      .action_primary = Rgb(0x1F, 0x6F, 0xEB),
      .action_primary_hover = Rgb(0x1A, 0x64, 0xD6),
      .action_primary_pressed = Rgb(0x15, 0x58, 0xBA),
      .on_emphasis = Rgb(0xFF, 0xFF, 0xFF),
      .information = Rgb(0x44, 0x93, 0xF8),
      .information_background = Rgb(0x17, 0x28, 0x3F),
      .success = Rgb(0x3F, 0xB9, 0x50),
      .success_background = Rgb(0x16, 0x2F, 0x21),
      .warning = Rgb(0xD2, 0x99, 0x22),
      .warning_background = Rgb(0x31, 0x2A, 0x19),
      .failure = Rgb(0xF8, 0x51, 0x49),
      .failure_background = Rgb(0x38, 0x1D, 0x1D),
  };
}

void ApplyTheme(const ResolvedTheme theme, const float ui_scale) {
  active_palette = PaletteFor(theme);
  active_ui_scale = std::clamp(ui_scale, 0.75f, 2.0f);
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowPadding =
      ImVec2(metrics.spacing.space05, metrics.spacing.space05);
  style.FramePadding = ImVec2(metrics.spacing.space04, Scale(6.0f));
  style.ItemSpacing = ImVec2(metrics.spacing.space03, metrics.spacing.space03);
  style.ItemInnerSpacing =
      ImVec2(metrics.spacing.space03, metrics.spacing.space02);
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.FrameRounding = metrics.geometry.control_radius;
  style.FrameBorderSize = metrics.geometry.border;
  style.PopupRounding = metrics.geometry.surface_radius;
  style.PopupBorderSize = metrics.geometry.border;
  style.TreeLinesSize = metrics.geometry.border;
  style.TreeLinesRounding = metrics.geometry.surface_radius;
  // Components supply explicit disabled colors. Keeping alpha at one prevents
  // Dear ImGui from washing those semantic roles out a second time.
  style.DisabledAlpha = 1.0f;

  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = ToImVec4(active_palette.text_primary);
  colors[ImGuiCol_TextDisabled] = ToImVec4(active_palette.text_disabled);
  colors[ImGuiCol_WindowBg] = ToImVec4(active_palette.application_surface);
  colors[ImGuiCol_MenuBarBg] = ToImVec4(active_palette.application_surface);
  colors[ImGuiCol_ChildBg] = ToImVec4(active_palette.surface);
  colors[ImGuiCol_PopupBg] = ToImVec4(active_palette.surface_raised);
  colors[ImGuiCol_Border] = ToImVec4(active_palette.border);
  colors[ImGuiCol_FrameBg] = ToImVec4(active_palette.control);
  colors[ImGuiCol_FrameBgHovered] = ToImVec4(active_palette.control_hover);
  colors[ImGuiCol_FrameBgActive] = ToImVec4(active_palette.control_pressed);
  colors[ImGuiCol_Button] = ToImVec4(active_palette.control);
  colors[ImGuiCol_ButtonHovered] = ToImVec4(active_palette.control_hover);
  colors[ImGuiCol_ButtonActive] = ToImVec4(active_palette.control_pressed);
  colors[ImGuiCol_Header] = ToImVec4(active_palette.selection);
  colors[ImGuiCol_HeaderHovered] = ToImVec4(active_palette.control_hover);
  colors[ImGuiCol_HeaderActive] = ToImVec4(active_palette.control_pressed);
  colors[ImGuiCol_TreeLines] = ToImVec4(active_palette.border_strong);
  colors[ImGuiCol_CheckMark] = ToImVec4(active_palette.action_primary);
  colors[ImGuiCol_SliderGrab] = ToImVec4(active_palette.action_primary);
  colors[ImGuiCol_SliderGrabActive] =
      ToImVec4(active_palette.action_primary_hover);
  colors[ImGuiCol_NavCursor] = ToImVec4(active_palette.focus);
}

const SemanticPalette &CurrentPalette() { return active_palette; }

float CurrentUiScale() { return active_ui_scale; }

float Scale(const float logical_pixels) {
  return logical_pixels * active_ui_scale;
}

} // namespace fancy_ui
