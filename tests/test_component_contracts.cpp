#include "fancy_ui/fancy_ui.hpp"
#include "fancy_ui/steppenface/ui_assets.hpp"
#include "internal/component_internal.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace {

float LinearChannel(const float channel) {
  return channel <= 0.04045f ? channel / 12.92f
                             : std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

float RelativeLuminance(const fancy_ui::ColorRgba color) {
  return 0.2126f * LinearChannel(color.red) +
         0.7152f * LinearChannel(color.green) +
         0.0722f * LinearChannel(color.blue);
}

float ContrastRatio(const fancy_ui::ColorRgba first,
                    const fancy_ui::ColorRgba second) {
  const float first_luminance = RelativeLuminance(first);
  const float second_luminance = RelativeLuminance(second);
  const float lighter = std::max(first_luminance, second_luminance);
  const float darker = std::min(first_luminance, second_luminance);
  return (lighter + 0.05f) / (darker + 0.05f);
}

fancy_ui::UiEnvironment EnvironmentAt(const float scale) {
  return {
      .layout_scale = scale,
      .raster_scale = scale,
  };
}

} // namespace

TEST_CASE("component availability carries the caller-provided reason") {
  const fancy_ui::Availability availability{
      .enabled = false,
      .reason = "Select at least one object",
  };

  REQUIRE_FALSE(availability.enabled);
  REQUIRE(availability.reason == "Select at least one object");
}

TEST_CASE("light and dark palettes expose distinct semantic surfaces") {
  const fancy_ui::SemanticPalette light =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Light);
  const fancy_ui::SemanticPalette dark =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark);

  REQUIRE(light.surface.red > dark.surface.red);
  REQUIRE(light.text_primary.red < dark.text_primary.red);
  REQUIRE(light.action_primary.alpha == 1.0f);
  REQUIRE(dark.action_primary.alpha == 1.0f);
  REQUIRE(light.control_disabled_fill.alpha == 1.0f);
  REQUIRE(dark.control_disabled_fill.alpha == 1.0f);
  REQUIRE(light.control_disabled_fill != light.control);
  REQUIRE(dark.control_disabled_fill != dark.control);
}

TEST_CASE("display scale and pixel density scale the UI exactly once") {
  fancy_ui::UiEnvironment environment;

  fancy_ui::UpdateDisplayScales(environment, 1.25f, 1.25f);
  REQUIRE(environment.layout_scale == Catch::Approx(1.0f));
  REQUIRE(environment.raster_scale == Catch::Approx(1.25f));

  fancy_ui::UpdateDisplayScales(environment, 1.25f, 1.0f);
  REQUIRE(environment.layout_scale == Catch::Approx(1.25f));
  REQUIRE(environment.raster_scale == Catch::Approx(1.25f));

  fancy_ui::UpdateDisplayScales(environment, 2.0f, 2.0f);
  REQUIRE(environment.layout_scale == Catch::Approx(1.0f));
  REQUIRE(environment.raster_scale == Catch::Approx(2.0f));

  fancy_ui::UpdateDisplayScales(environment, 5.0f, 1.0f);
  REQUIRE(environment.layout_scale == Catch::Approx(5.0f));
  REQUIRE(environment.raster_scale == Catch::Approx(5.0f));
}

TEST_CASE("the UI font manifest exposes every upright Noto weight") {
  const auto fonts = fancy_ui::steppenface::RequiredUiFontAssets();
  std::set<fancy_ui::UiFontWeight> weights;
  std::size_t monospace_count = 0;
  for (const fancy_ui::steppenface::UiFontAssetSpec &font : fonts) {
    if (font.monospace) {
      ++monospace_count;
    } else {
      weights.insert(font.weight);
    }
  }

  const std::set expected_weights{
      fancy_ui::UiFontWeight::Thin,   fancy_ui::UiFontWeight::ExtraLight,
      fancy_ui::UiFontWeight::Light,  fancy_ui::UiFontWeight::Regular,
      fancy_ui::UiFontWeight::Medium, fancy_ui::UiFontWeight::SemiBold,
      fancy_ui::UiFontWeight::Bold,   fancy_ui::UiFontWeight::ExtraBold,
      fancy_ui::UiFontWeight::Black,
  };
  REQUIRE(weights == expected_weights);
  REQUIRE(monospace_count == 1);
}

TEST_CASE("operation and destructive control colors meet contrast targets") {
  ImGui::CreateContext();
  for (const fancy_ui::ResolvedTheme theme :
       {fancy_ui::ResolvedTheme::Light, fancy_ui::ResolvedTheme::Dark}) {
    fancy_ui::ApplyTheme(theme);
    const fancy_ui::SemanticPalette palette = fancy_ui::PaletteFor(theme);

    REQUIRE(ContrastRatio(palette.text_primary, palette.surface_muted) >= 7.0f);
    REQUIRE(palette.text_secondary != palette.text_disabled);
    for (const fancy_ui::ColorRgba surface :
         {palette.application_surface, palette.canvas, palette.surface,
          palette.surface_muted, palette.surface_raised}) {
      REQUIRE(ContrastRatio(palette.text_secondary, surface) >= 4.5f);
    }
    for (const fancy_ui::ColorRgba background :
         {palette.information_background, palette.success_background,
          palette.warning_background, palette.failure_background}) {
      REQUIRE(ContrastRatio(palette.text_primary, background) >= 4.5f);
    }

    for (const fancy_ui::detail::ControlState state :
         {fancy_ui::detail::ControlState{.destructive = true},
          fancy_ui::detail::ControlState{
              .hovered = true,
              .destructive = true,
          },
          fancy_ui::detail::ControlState{
              .hovered = true,
              .pressed = true,
              .destructive = true,
          }}) {
      const fancy_ui::detail::ControlColors colors =
          fancy_ui::detail::ResolveControlColors(state);
      REQUIRE(colors.text == palette.text_primary);
      REQUIRE(colors.border == palette.failure);
      REQUIRE(ContrastRatio(colors.text, colors.fill) >= 4.5f);
      REQUIRE(ContrastRatio(colors.border, colors.fill) >= 3.0f);
    }
  }
  ImGui::DestroyContext();
}

TEST_CASE("theme scale supports the platform scale without an upper clamp") {
  ImGui::CreateContext();

  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark, EnvironmentAt(3.0f));
  REQUIRE(fancy_ui::CurrentUiScale() == Catch::Approx(3.0f));
  REQUIRE(ImGui::GetStyle().FramePadding.x == Catch::Approx(36.0f));
  REQUIRE(ImGui::GetStyle().ItemSpacing.y == Catch::Approx(24.0f));
  REQUIRE(ImGui::GetStyle().ButtonTextAlign.x == Catch::Approx(0.5f));
  REQUIRE(ImGui::GetStyle().ButtonTextAlign.y == Catch::Approx(0.5f));
  REQUIRE(ImGui::GetStyle().DisabledAlpha == Catch::Approx(1.0f));
  REQUIRE(ImGui::GetStyle().ChildRounding == Catch::Approx(12.0f));
  REQUIRE(ImGui::GetStyle().ScrollbarSize == Catch::Approx(30.0f));
  REQUIRE(ImGui::GetStyle().TabBarOverlineSize == Catch::Approx(9.0f));
  const fancy_ui::SemanticPalette dark =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark);
  const ImVec4 tree_lines = ImGui::GetStyle().Colors[ImGuiCol_TreeLines];
  REQUIRE(tree_lines.x == Catch::Approx(dark.border_strong.red));
  REQUIRE(tree_lines.y == Catch::Approx(dark.border_strong.green));
  REQUIRE(tree_lines.z == Catch::Approx(dark.border_strong.blue));
  const ImVec4 title = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
  REQUIRE(title.x == Catch::Approx(dark.surface_raised.red));
  REQUIRE(title.y == Catch::Approx(dark.surface_raised.green));
  REQUIRE(title.z == Catch::Approx(dark.surface_raised.blue));
  const ImVec4 selected_tab = ImGui::GetStyle().Colors[ImGuiCol_TabSelected];
  REQUIRE(selected_tab.x == Catch::Approx(dark.selection.red));
  REQUIRE(selected_tab.y == Catch::Approx(dark.selection.green));
  REQUIRE(selected_tab.z == Catch::Approx(dark.selection.blue));

  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Light, EnvironmentAt(0.5f));
  REQUIRE(fancy_ui::CurrentUiScale() == Catch::Approx(0.5f));
  REQUIRE(fancy_ui::Scale(32.0f) == Catch::Approx(16.0f));
  ImGui::DestroyContext();
}

TEST_CASE("layout metrics expose the normative shell and panel geometry") {
  const fancy_ui::LayoutMetrics &metrics = fancy_ui::LogicalLayoutMetrics();

  REQUIRE(metrics.shell.application_bar_height == 40.0f);
  REQUIRE(metrics.shell.context_toolbar_height == 40.0f);
  REQUIRE(metrics.shell.activity_rail_width == 48.0f);
  REQUIRE(metrics.shell.explorer_width == 256.0f);
  REQUIRE(metrics.shell.explorer_minimum_width == 240.0f);
  REQUIRE(metrics.shell.explorer_maximum_width == 280.0f);
  REQUIRE(metrics.shell.splitter_width == 8.0f);
  REQUIRE(metrics.shell.inspector_width == 320.0f);
  REQUIRE(metrics.shell.inspector_minimum_width == 300.0f);
  REQUIRE(metrics.shell.inspector_maximum_width == 360.0f);
  REQUIRE(metrics.shell.operation_tray_minimum_height == 160.0f);
  REQUIRE(metrics.shell.operation_tray_maximum_height == 240.0f);
  REQUIRE(metrics.shell.operation_strip_height == 32.0f);
  REQUIRE(metrics.shell.status_bar_height == 24.0f);

  REQUIRE(metrics.geometry.progress_height == 6.0f);
  REQUIRE(metrics.geometry.row_height == 32.0f);
  REQUIRE(metrics.typography.body_font_height == Catch::Approx(18.16f));
  REQUIRE(metrics.typography.section_heading_font_height ==
          Catch::Approx(20.43f));
  REQUIRE(metrics.explorer.tree_indent == 16.0f);
  REQUIRE(metrics.inspector.label_width == 112.0f);
  REQUIRE(metrics.inspector.stack_breakpoint == 288.0f);
  REQUIRE(metrics.menu.popup_width == 264.0f);
  REQUIRE(metrics.settings.title_bar_height == 48.0f);
}

TEST_CASE("resolved layout metrics scale geometry once") {
  const fancy_ui::LayoutMetrics minimum =
      fancy_ui::ResolveLayoutMetrics(EnvironmentAt(0.5f));
  const fancy_ui::LayoutMetrics fractional =
      fancy_ui::ResolveLayoutMetrics(EnvironmentAt(1.25f));
  const fancy_ui::LayoutMetrics maximum =
      fancy_ui::ResolveLayoutMetrics(EnvironmentAt(3.0f));

  REQUIRE(minimum.shell.application_bar_height == 20.0f);
  REQUIRE(fractional.spacing.condensed == 1.0f);
  REQUIRE(fractional.geometry.focus_ring == 3.0f);
  REQUIRE(fractional.shell.explorer_width == 320.0f);
  REQUIRE(fractional.geometry.row_height == 40.0f);
  REQUIRE(fractional.typography.body_font_height == Catch::Approx(22.7f));
  REQUIRE(fractional.explorer.tree_indent == 20.0f);
  REQUIRE(maximum.shell.inspector_width == 960.0f);
}

TEST_CASE("default progress bars flex and use the shared height") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640.0f, 480.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int texture_width = 0;
  int texture_height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &texture_width, &texture_height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  const auto draw = [](const float window_width) {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(window_width, 80.0f), ImGuiCond_Always);
    ImGui::Begin("progress-contract", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    fancy_ui::ProgressBar({
        .id = "progress",
        .label = "Search progress",
        .value = 0.62f,
    });
    const ImVec2 size = ImGui::GetItemRectSize();
    ImGui::End();
    ImGui::Render();
    return size;
  };

  const ImVec2 narrow = draw(240.0f);
  const ImVec2 wide = draw(440.0f);
  REQUIRE(wide.x > narrow.x);
  REQUIRE(
      narrow.y ==
      Catch::Approx(fancy_ui::CurrentLayoutMetrics().geometry.progress_height));
  ImGui::DestroyContext();
}

TEST_CASE("shared tooltips use eight scaled pixels without changing windows") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(320.0f, 240.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark, EnvironmentAt(1.5f));
  const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;

  ImGui::NewFrame();
  ImGui::Begin("tooltip-contract");
  fancy_ui::detail::ShowTooltip("Tooltip content");
  REQUIRE(ImGui::GetStyle().WindowPadding.x == Catch::Approx(window_padding.x));
  REQUIRE(ImGui::GetStyle().WindowPadding.y == Catch::Approx(window_padding.y));
  ImGui::End();
  ImGui::Render();

  ImGuiWindow *tooltip = ImGui::FindWindowByName("##Tooltip_00");
  REQUIRE(tooltip != nullptr);
  REQUIRE(tooltip->WindowPadding.x == Catch::Approx(12.0f));
  REQUIRE(tooltip->WindowPadding.y == Catch::Approx(12.0f));
  ImGui::DestroyContext();
}

TEST_CASE("compact buttons center labels without leaking frame padding") {
  REQUIRE(fancy_ui::detail::ResolveButtonVerticalPadding(0.0f, 16.0f, 6.0f) ==
          Catch::Approx(6.0f));
  REQUIRE(fancy_ui::detail::ResolveButtonVerticalPadding(32.0f, 16.0f, 6.0f) ==
          Catch::Approx(6.0f));
  REQUIRE(fancy_ui::detail::ResolveButtonVerticalPadding(24.0f, 16.0f, 6.0f) ==
          Catch::Approx(4.0f));
  REQUIRE(fancy_ui::detail::ResolveButtonVerticalPadding(12.0f, 16.0f, 6.0f) ==
          Catch::Approx(0.0f));

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(320.0f, 240.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);
  const ImVec2 frame_padding = ImGui::GetStyle().FramePadding;

  ImGui::NewFrame();
  ImGui::Begin("compact-button-contract");
  const float font_size = ImGui::GetFontSize();
  static_cast<void>(fancy_ui::Button({
      .id = "compact",
      .label = "Compact",
      .size = {.x = 96.0f, .y = 24.0f},
  }));
  const float restored_font_size = ImGui::GetFontSize();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImGui::End();
  ImGui::Render();

  REQUIRE(maximum.y - minimum.y == Catch::Approx(24.0f));
  REQUIRE(restored_font_size == Catch::Approx(font_size));
  REQUIRE(ImGui::GetStyle().FramePadding.x == Catch::Approx(frame_padding.x));
  REQUIRE(ImGui::GetStyle().FramePadding.y == Catch::Approx(frame_padding.y));
  ImGui::DestroyContext();
}

TEST_CASE("rotation compass derives evenly spaced canonical angles") {
  REQUIRE(fancy_ui::ClampRotationCount(-8) == fancy_ui::kRotationCountMinimum);
  REQUIRE(fancy_ui::ClampRotationCount(42) == fancy_ui::kRotationCountMaximum);
  REQUIRE(fancy_ui::RotationStepDegrees(4) == Catch::Approx(90.0));
  REQUIRE(fancy_ui::FormatRotationDegrees(22.5) == "22.5°");

  const std::vector<double> angles = fancy_ui::EvenlySpacedRotationAngles(4);
  REQUIRE(angles == std::vector<double>{0.0, 90.0, 180.0, 270.0});
}

TEST_CASE("shared slider thumb stays under the mouse while dragging") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(480.0f, 180.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int texture_width = 0;
  int texture_height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &texture_width, &texture_height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  float value = 38.0f;
  ImVec2 item_minimum;
  ImVec2 item_maximum;
  const auto draw_slider = [&]() {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::Begin("slider-drag", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::SetNextItemWidth(240.0f);
    const bool changed = fancy_ui::detail::DrawSliderFloat(
        "explode", value, 0.0f, 100.0f, "%.0f", "%", true, true);
    item_minimum = ImGui::GetItemRectMin();
    item_maximum = ImGui::GetItemRectMax();
    ImGui::End();
    ImGui::Render();
    return changed;
  };

  static_cast<void>(draw_slider());
  const fancy_ui::LayoutMetrics metrics = fancy_ui::CurrentLayoutMetrics();
  const float thumb_inset = 2.0f + metrics.geometry.icon * 0.5f;
  const float track_minimum =
      item_minimum.x + metrics.spacing.space03 + thumb_inset;
  const float output_width = ImGui::CalcTextSize("38%").x;
  const float track_maximum = item_maximum.x - metrics.spacing.space03 -
                              output_width - metrics.spacing.space03 -
                              thumb_inset;
  const float track_y = (item_minimum.y + item_maximum.y) * 0.5f;
  const float thumb_x = track_minimum + (track_maximum - track_minimum) * 0.38f;

  io.AddMousePosEvent(thumb_x, track_y);
  static_cast<void>(draw_slider());
  io.AddMouseButtonEvent(0, true);
  REQUIRE_FALSE(draw_slider());
  REQUIRE(value == Catch::Approx(38.0f));

  const float target_x =
      track_minimum + (track_maximum - track_minimum) * 0.75f;
  io.AddMousePosEvent(target_x, track_y);
  REQUIRE(draw_slider());
  REQUIRE(value == Catch::Approx(75.0f));

  io.AddMouseButtonEvent(0, false);
  static_cast<void>(draw_slider());
  ImGui::DestroyContext();
}

TEST_CASE("system theme falls back to dark without a platform preference") {
  REQUIRE(fancy_ui::ResolveTheme(fancy_ui::ThemeMode::System) ==
          fancy_ui::ResolvedTheme::Dark);
  REQUIRE(fancy_ui::ResolveTheme(fancy_ui::ThemeMode::System,
                                 fancy_ui::ResolvedTheme::Light) ==
          fancy_ui::ResolvedTheme::Light);
}

TEST_CASE("menu bar background resolves to the application surface") {
  ImGui::CreateContext();
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  const fancy_ui::SemanticPalette palette =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark);
  const ImVec4 menu_bar = ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg];

  REQUIRE(menu_bar.x == Catch::Approx(palette.application_surface.red));
  REQUIRE(menu_bar.y == Catch::Approx(palette.application_surface.green));
  REQUIRE(menu_bar.z == Catch::Approx(palette.application_surface.blue));
  ImGui::DestroyContext();
}

TEST_CASE("shell state starts with independently visible side panels") {
  const fancy_ui::shell::ApplicationShellState state;

  REQUIRE(state.explorer_visible);
  REQUIRE(state.inspector_visible);
  REQUIRE(state.explorer_width == 256.0f);
  REQUIRE(state.inspector_width == 320.0f);
}

TEST_CASE("inline application menu bar occupies the full shell region") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  float region_height = 0.0f;
  float menu_bar_height = 0.0f;
  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("inline-application-menu-bar", nullptr,
               ImGuiWindowFlags_NoDecoration);
  ImGui::PopStyleVar();

  const fancy_ui::shell::ApplicationShellSpec spec{
      .application_bar =
          {
              .id = "application-bar",
              .draw =
                  [&region_height, &menu_bar_height]() {
                    ImGuiWindow *window = ImGui::GetCurrentWindow();
                    region_height = window->Size.y;
                    if (ImGui::BeginMenuBar()) {
                      menu_bar_height = window->MenuBarRect().GetHeight();
                      ImGui::EndMenuBar();
                    }
                  },
              .menu_bar = true,
              .zero_padding = true,
          },
  };
  static_cast<void>(fancy_ui::shell::Application(spec, {}));
  ImGui::End();
  ImGui::Render();

  REQUIRE(region_height == Catch::Approx(40.0f));
  REQUIRE(menu_bar_height == Catch::Approx(region_height));
  ImGui::DestroyContext();
}

TEST_CASE("navigation items provide a 48 pixel target and 24 pixel icon slot") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(320.0f, 240.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  fancy_ui::Rect icon_bounds;
  ImGui::NewFrame();
  ImGui::Begin("navigation-contract");
  (void)fancy_ui::NavigationItem({
      .id = "objects",
      .label = "Objects",
      .selected = true,
      .draw_icon =
          [&icon_bounds](const fancy_ui::Rect &bounds,
                         const fancy_ui::ColorRgba) { icon_bounds = bounds; },
  });
  const ImVec2 target_minimum = ImGui::GetItemRectMin();
  const ImVec2 target_maximum = ImGui::GetItemRectMax();
  ImGui::End();
  ImGui::Render();

  REQUIRE(target_maximum.x - target_minimum.x == Catch::Approx(48.0f));
  REQUIRE(target_maximum.y - target_minimum.y == Catch::Approx(48.0f));
  REQUIRE(icon_bounds.maximum.x - icon_bounds.minimum.x ==
          Catch::Approx(24.0f));
  REQUIRE(icon_bounds.maximum.y - icon_bounds.minimum.y ==
          Catch::Approx(24.0f));
  ImGui::DestroyContext();
}

TEST_CASE("logical control dimensions follow the configured UI scale") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640.0f, 480.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark, EnvironmentAt(2.0f));

  ImGui::NewFrame();
  ImGui::Begin("scaled-control");
  static_cast<void>(fancy_ui::Button({
      .id = "scaled",
      .label = "Scaled",
      .size = {.x = 80.0f, .y = 32.0f},
  }));
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImGui::End();
  ImGui::Render();

  REQUIRE(maximum.x - minimum.x == Catch::Approx(160.0f));
  REQUIRE(maximum.y - minimum.y == Catch::Approx(64.0f));
  ImGui::DestroyContext();
}

TEST_CASE("gallery field layout preview keeps narrow fields inline only while "
          "scoped") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640.0f, 480.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  ImGui::NewFrame();
  ImGui::SetNextWindowSize(ImVec2(278.0f, 240.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("field-layout-preview", nullptr, ImGuiWindowFlags_NoDecoration);
  ImGui::PopStyleVar();

  const fancy_ui::detail::FieldLayout stacked =
      fancy_ui::detail::BeginFieldLayout("Spacing");
  fancy_ui::detail::EndFieldLayout(stacked, {});
  bool inline_preview = false;
  {
    const fancy_ui::detail::ScopedFieldLayoutPreview preview(76.0f);
    const fancy_ui::detail::FieldLayout layout =
        fancy_ui::detail::BeginFieldLayout("Spacing");
    inline_preview = layout.table;
    fancy_ui::detail::EndFieldLayout(layout, {});
  }
  const fancy_ui::detail::FieldLayout restored =
      fancy_ui::detail::BeginFieldLayout("Spacing");
  fancy_ui::detail::EndFieldLayout(restored, {});

  ImGui::End();
  ImGui::Render();

  REQUIRE_FALSE(stacked.table);
  REQUIRE(inline_preview);
  REQUIRE_FALSE(restored.table);
  ImGui::DestroyContext();
}

TEST_CASE("duration and mixed toggle states remain within their contracts") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640.0f, 480.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  ImGui::NewFrame();
  ImGui::Begin("state-contracts");
  const fancy_ui::DurationResult duration = fancy_ui::Duration({
      .id = "duration",
      .label = "Duration",
      .hours = 99,
      .minutes = -4,
  });
  const fancy_ui::CheckboxResult checkbox = fancy_ui::Checkbox({
      .id = "mixed",
      .label = "Mixed",
      .state = fancy_ui::ToggleState::Mixed,
  });
  ImGui::End();
  ImGui::Render();

  REQUIRE(duration.hours == 23);
  REQUIRE(duration.minutes == 0);
  REQUIRE_FALSE(duration.changed);
  REQUIRE(checkbox.state == fancy_ui::ToggleState::Mixed);
  REQUIRE_FALSE(checkbox.changed);
  ImGui::DestroyContext();
}

TEST_CASE("hierarchy visibility reducers preserve mixed group meaning") {
  using fancy_ui::ToggleState;

  REQUIRE(fancy_ui::AggregateVisibility(std::span<const ToggleState>{}) ==
          ToggleState::Off);
  static constexpr std::array all_visible{ToggleState::On, ToggleState::On};
  static constexpr std::array all_hidden{ToggleState::Off, ToggleState::Off};
  static constexpr std::array mixed{ToggleState::On, ToggleState::Off};
  static constexpr std::array nested_mixed{ToggleState::On, ToggleState::Mixed};
  REQUIRE(fancy_ui::AggregateVisibility(all_visible) == ToggleState::On);
  REQUIRE(fancy_ui::AggregateVisibility(all_hidden) == ToggleState::Off);
  REQUIRE(fancy_ui::AggregateVisibility(mixed) == ToggleState::Mixed);
  REQUIRE(fancy_ui::AggregateVisibility(nested_mixed) == ToggleState::Mixed);
  REQUIRE(fancy_ui::NextVisibilityState(ToggleState::On) == ToggleState::Off);
  REQUIRE(fancy_ui::NextVisibilityState(ToggleState::Off) == ToggleState::On);
  REQUIRE(fancy_ui::NextVisibilityState(ToggleState::Mixed) == ToggleState::On);
}

TEST_CASE("color swatch opens a transactional picker and commits on Enter") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.DisplaySize = ImVec2(640.0f, 480.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  fancy_ui::ColorRgba value{
      .red = 0.2f,
      .green = 0.4f,
      .blue = 0.6f,
  };
  fancy_ui::ColorPickerLayout layout =
      fancy_ui::ColorPickerLayout::CurrentAndOriginal;
  SECTION("current and original layout") {
    layout = fancy_ui::ColorPickerLayout::CurrentAndOriginal;
  }
  SECTION("compact layout") { layout = fancy_ui::ColorPickerLayout::Compact; }
  fancy_ui::ColorPickerState picker;
  fancy_ui::ColorSwatchResult result;
  ImVec2 swatch_minimum;
  ImVec2 swatch_maximum;
  const auto draw = [&] {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(260.0f, 420.0f), ImGuiCond_Always);
    ImGui::Begin("color-contract", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    result = fancy_ui::ColorSwatch(
        {
            .id = "color",
            .label = "Bed color",
            .value = value,
            .colors = std::span<const fancy_ui::ColorRgba>(&value, 1),
            .picker_layout = layout,
        },
        picker);
    if (!picker.editing) {
      swatch_minimum = ImGui::GetItemRectMin();
      swatch_maximum = ImGui::GetItemRectMax();
    }
    ImGui::End();
    ImGui::Render();
  };

  draw();
  const ImVec2 swatch_center((swatch_minimum.x + swatch_maximum.x) * 0.5f,
                             (swatch_minimum.y + swatch_maximum.y) * 0.5f);
  io.AddMousePosEvent(swatch_center.x, swatch_center.y);
  draw();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  draw();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
  draw();
  REQUIRE(result.activated);
  REQUIRE(result.picker_open);
  REQUIRE(picker.editing);

  picker.draft = {
      .red = 0.8f,
      .green = 0.3f,
      .blue = 0.1f,
      .alpha = 0.75f,
  };
  io.AddKeyEvent(ImGuiKey_Enter, true);
  draw();
  REQUIRE(result.changed);
  REQUIRE(result.committed);
  REQUIRE_FALSE(result.cancelled);
  REQUIRE_FALSE(result.picker_open);
  REQUIRE(result.value == picker.draft);
  REQUIRE(picker.restore_focus);
  value = result.value;
  io.AddKeyEvent(ImGuiKey_Enter, false);
  draw();

  io.AddMousePosEvent(swatch_center.x, swatch_center.y);
  draw();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  draw();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
  draw();
  REQUIRE(result.picker_open);
  picker.draft = {
      .red = 0.1f,
      .green = 0.9f,
      .blue = 0.4f,
  };
  io.AddKeyEvent(ImGuiKey_Escape, true);
  draw();
  REQUIRE(result.cancelled);
  REQUIRE_FALSE(result.committed);
  REQUIRE_FALSE(result.changed);
  REQUIRE(result.value == value);
  REQUIRE_FALSE(result.picker_open);
  io.AddKeyEvent(ImGuiKey_Escape, false);

  ImGui::DestroyContext();
}

TEST_CASE("hierarchy inline targets do not activate the selectable row") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640.0f, 480.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  const fancy_ui::IconPainter icon = [](const fancy_ui::Rect &,
                                        const fancy_ui::ColorRgba) {};
  fancy_ui::HierarchyRowResult result;
  ImVec2 row_minimum;
  ImVec2 row_maximum;
  const auto draw = [&] {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 80.0f), ImGuiCond_Always);
    ImGui::Begin("hierarchy-contract", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    row_minimum = ImGui::GetCursorScreenPos();
    row_maximum = ImVec2(
        row_minimum.x + ImGui::GetContentRegionAvail().x,
        row_minimum.y + fancy_ui::CurrentLayoutMetrics().geometry.row_height);
    {
      fancy_ui::HierarchyTree tree;
      result = fancy_ui::HierarchyRow(
          tree, {
                    .id = "part",
                    .label = "Face plate",
                    .metadata = "Part 4",
                    .expandable = true,
                    .expanded = true,
                    .color =
                        fancy_ui::ColorRgba{
                            .red = 0.27f,
                            .green = 0.58f,
                            .blue = 0.97f,
                        },
                    .action_icon = icon,
                    .visibility = fancy_ui::ToggleState::On,
                    .visible_icon = icon,
                    .hidden_icon = icon,
                });
      if (result.expanded) {
        tree.Pop();
      }
    }
    ImGui::End();
    ImGui::Render();
  };
  const auto click = [&](const ImVec2 point) {
    io.AddMousePosEvent(point.x, point.y);
    draw();
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    draw();
    fancy_ui::HierarchyRowResult clicked = result;
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    draw();
    clicked.activated |= result.activated;
    clicked.expansion_changed |= result.expansion_changed;
    clicked.color_activated |= result.color_activated;
    clicked.action_activated |= result.action_activated;
    clicked.visibility_changed |= result.visibility_changed;
    clicked.expanded = result.expanded;
    clicked.visibility = result.visibility;
    return clicked;
  };

  draw();
  const float center_y = (row_minimum.y + row_maximum.y) * 0.5f;
  const fancy_ui::HierarchyRowResult row =
      click(ImVec2(row_minimum.x + 100.0f, center_y));
  REQUIRE(row.activated);
  REQUIRE_FALSE(row.expansion_changed);
  REQUIRE_FALSE(row.color_activated);
  REQUIRE_FALSE(row.action_activated);
  REQUIRE_FALSE(row.visibility_changed);

  const fancy_ui::HierarchyRowResult expander =
      click(ImVec2(row_minimum.x + fancy_ui::Scale(20.0f), center_y));
  REQUIRE_FALSE(expander.activated);
  REQUIRE(expander.expansion_changed);

  const fancy_ui::HierarchyRowResult color =
      click(ImVec2(row_maximum.x - fancy_ui::Scale(60.0f), center_y));
  REQUIRE_FALSE(color.activated);
  REQUIRE(color.color_activated);

  const fancy_ui::HierarchyRowResult visibility =
      click(ImVec2(row_maximum.x - fancy_ui::Scale(36.0f), center_y));
  REQUIRE_FALSE(visibility.activated);
  REQUIRE(visibility.visibility_changed);
  REQUIRE(visibility.visibility == fancy_ui::ToggleState::Off);

  const fancy_ui::HierarchyRowResult action =
      click(ImVec2(row_maximum.x - fancy_ui::Scale(12.0f), center_y));
  REQUIRE_FALSE(action.activated);
  REQUIRE(action.action_activated);
  ImGui::DestroyContext();
}

TEST_CASE("sectioned hierarchy roots and children use contiguous 32 px rows") {
  const auto verify_scale = [](const float scale) {
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 1024.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
    unsigned char *pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark, EnvironmentAt(scale));

    ImVec2 parent_minimum;
    ImVec2 parent_maximum;
    ImVec2 child_minimum;
    ImVec2 child_maximum;
    ImVec2 next_root_minimum;
    ImVec2 next_root_maximum;
    float scoped_spacing = 0.0f;
    float restored_spacing = 0.0f;
    bool parent_expanded = false;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(840.0f, 320.0f), ImGuiCond_Always);
    ImGui::Begin("hierarchy-density", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    {
      fancy_ui::HierarchyTree tree;
      scoped_spacing = ImGui::GetStyle().ItemSpacing.y;
      const fancy_ui::HierarchyRowResult parent =
          fancy_ui::HierarchyRow(tree, {
                                           .id = "parent",
                                           .label = "Parent",
                                           .metadata = "3 items",
                                           .expandable = true,
                                           .expanded = true,
                                       });
      parent_expanded = parent.expanded;
      parent_minimum = ImGui::GetItemRectMin();
      parent_maximum = ImGui::GetItemRectMax();
      static_cast<void>(fancy_ui::HierarchyRow(tree, {
                                                         .id = "child",
                                                         .label = "Child",
                                                         .metadata = "Ready",
                                                     }));
      child_minimum = ImGui::GetItemRectMin();
      child_maximum = ImGui::GetItemRectMax();
      tree.Pop();
    }
    {
      fancy_ui::HierarchyTree tree;
      static_cast<void>(fancy_ui::HierarchyRow(tree, {
                                                         .id = "next-root",
                                                         .label = "Next root",
                                                     }));
      next_root_minimum = ImGui::GetItemRectMin();
      next_root_maximum = ImGui::GetItemRectMax();
    }
    restored_spacing = ImGui::GetStyle().ItemSpacing.y;
    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext();

    REQUIRE(parent_expanded);
    REQUIRE(parent_maximum.y - parent_minimum.y ==
            Catch::Approx(32.0f * scale));
    REQUIRE(child_maximum.y - child_minimum.y == Catch::Approx(32.0f * scale));
    REQUIRE(next_root_maximum.y - next_root_minimum.y ==
            Catch::Approx(32.0f * scale));
    REQUIRE(child_minimum.y - parent_maximum.y == Catch::Approx(0.0f));
    REQUIRE(child_minimum.x == Catch::Approx(parent_minimum.x));
    REQUIRE(scoped_spacing == Catch::Approx(0.0f));
    REQUIRE(restored_spacing == Catch::Approx(8.0f * scale));
  };

  verify_scale(1.0f);
  verify_scale(2.0f);
}

TEST_CASE("information tree rows expose disclosure and aggregate visibility") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640.0f, 480.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  const fancy_ui::IconPainter icon = [](const fancy_ui::Rect &,
                                        const fancy_ui::ColorRgba) {};
  fancy_ui::InformationTreeRowResult result;
  ImVec2 row_minimum;
  ImVec2 row_maximum;
  const auto draw = [&] {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 100.0f), ImGuiCond_Always);
    ImGui::Begin("information-tree-contract", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    {
      fancy_ui::InformationTree tree;
      row_minimum = ImGui::GetCursorScreenPos();
      row_maximum =
          ImVec2(row_minimum.x + ImGui::GetContentRegionAvail().x,
                 row_minimum.y + fancy_ui::CurrentLayoutMetrics()
                                     .inspector.information_row_minimum_height);
      result = fancy_ui::InformationTreeRow(
          tree, {
                    .id = "repairable",
                    .label = "Repairable",
                    .metadata = "5",
                    .expandable = true,
                    .expanded = true,
                    .status = fancy_ui::SemanticStatus::Information,
                    .visibility = fancy_ui::ToggleState::Mixed,
                    .visible_icon = icon,
                    .hidden_icon = icon,
                });
      if (result.expanded) {
        tree.Pop();
      }
    }
    ImGui::End();
    ImGui::Render();
  };

  draw();
  REQUIRE(result.expanded);
  REQUIRE_FALSE(result.visibility_changed);

  const auto click = [&](const ImVec2 point) {
    io.AddMousePosEvent(point.x, point.y);
    draw();
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    draw();
    fancy_ui::InformationTreeRowResult clicked = result;
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    draw();
    clicked.expansion_changed |= result.expansion_changed;
    clicked.visibility_changed |= result.visibility_changed;
    if (result.visibility_changed) {
      clicked.visibility = result.visibility;
    }
    return clicked;
  };

  const fancy_ui::InformationTreeRowResult disclosure =
      click(ImVec2(row_minimum.x + fancy_ui::Scale(20.0f),
                   (row_minimum.y + row_maximum.y) * 0.5f));
  REQUIRE(disclosure.expansion_changed);
  REQUIRE_FALSE(disclosure.visibility_changed);

  const ImVec2 visibility_target(row_maximum.x - fancy_ui::Scale(24.0f),
                                 (row_minimum.y + row_maximum.y) * 0.5f);
  const fancy_ui::InformationTreeRowResult clicked = click(visibility_target);

  REQUIRE(clicked.visibility_changed);
  REQUIRE(clicked.visibility == fancy_ui::ToggleState::On);
  REQUIRE_FALSE(clicked.expansion_changed);
  ImGui::DestroyContext();
}

TEST_CASE("color picker layouts fit their popup work area at the right edge") {
  const auto verify_layout = [](const fancy_ui::ColorPickerLayout layout,
                                const float scale) {
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.DisplaySize = ImVec2(1280.0f, 1024.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
    unsigned char *pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark, EnvironmentAt(scale));
    io.AddMousePosEvent(1260.0f, 100.0f);

    fancy_ui::ColorPickerState picker;
    const auto draw = [&](const bool request_open) {
      ImGui::NewFrame();
      ImGui::SetNextWindowPos(ImVec2(1040.0f, 32.0f), ImGuiCond_Always);
      ImGui::SetNextWindowSize(ImVec2(220.0f, 180.0f), ImGuiCond_Always);
      ImGui::Begin("picker-layout-contract", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoSavedSettings);
      static_cast<void>(fancy_ui::ColorPickerPopup(
          {
              .id = "picker",
              .title = layout == fancy_ui::ColorPickerLayout::CurrentAndOriginal
                           ? "Current and original"
                           : "Compact",
              .value =
                  {
                      .red = 0.3f,
                      .green = 0.5f,
                      .blue = 0.8f,
                      .alpha = 0.75f,
                  },
              .request_open = request_open,
              .layout = layout,
          },
          picker));
      ImGui::End();
      ImGui::Render();
    };

    draw(true);
    draw(false);
    draw(false);

    REQUIRE_FALSE(GImGui->OpenPopupStack.empty());
    ImGuiWindow *popup = GImGui->OpenPopupStack.back().Window;
    REQUIRE(popup != nullptr);
    REQUIRE(popup->ContentSize.x <=
            popup->WorkRect.GetWidth() + fancy_ui::Scale(1.0f));
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    REQUIRE(popup->Pos.x >= viewport->WorkPos.x);
    REQUIRE(popup->Pos.x + popup->Size.x <=
            viewport->WorkPos.x + viewport->WorkSize.x + fancy_ui::Scale(1.0f));

    ImGui::DestroyContext();
  };

  verify_layout(fancy_ui::ColorPickerLayout::CurrentAndOriginal, 1.0f);
  verify_layout(fancy_ui::ColorPickerLayout::Compact, 1.0f);
  verify_layout(fancy_ui::ColorPickerLayout::CurrentAndOriginal, 2.0f);
  verify_layout(fancy_ui::ColorPickerLayout::Compact, 2.0f);
}

TEST_CASE("UI icon manifest has unique size variants backed by SVG masters") {
  using namespace fancy_ui::steppenface;
  const std::filesystem::path icon_root =
      std::filesystem::path(FANCY_UI_TEST_SOURCE_ROOT) / "assets" / "ui" /
      "icons";
  std::set<std::pair<std::string, int>> keys;
  std::set<std::string> rail_icons;
  std::set<std::string> small_icons;

  for (const UiIconAssetSpec &asset : UiIconAssets()) {
    REQUIRE(
        keys.emplace(std::string(asset.semantic_id), LogicalPixels(asset.size))
            .second);
    const std::filesystem::path path = icon_root / std::string(asset.filename);
    REQUIRE(std::filesystem::is_regular_file(path));
    std::ifstream input(path);
    std::ostringstream contents;
    contents << input.rdbuf();
    REQUIRE(contents.str().find(
                "viewBox=\"0 0 " + std::to_string(LogicalPixels(asset.size)) +
                " " + std::to_string(LogicalPixels(asset.size)) + "\"") !=
            std::string::npos);
    if (asset.size == UiIconSize::Rail24) {
      rail_icons.emplace(asset.semantic_id);
    } else {
      small_icons.emplace(asset.semantic_id);
    }
  }

  for (const char *semantic_id :
       {"model", "bed", "objects", "grain", "search", "compact", "diagnostics",
        "svg", "dxf", "path", "line", "arc", "circle", "select-children"}) {
    REQUIRE(rail_icons.contains(semantic_id));
  }
  for (const char *semantic_id :
       {"svg", "dxf", "path", "line", "arc", "circle", "select-children"}) {
    REQUIRE(small_icons.contains(semantic_id));
  }
  for (const char *semantic_id : {"information",
                                  "success",
                                  "alert",
                                  "failure",
                                  "busy",
                                  "check",
                                  "chevron-down",
                                  "triangle-down",
                                  "visibility",
                                  "visibility-off",
                                  "more",
                                  "focus",
                                  "orbit-locked",
                                  "orbit-unlocked",
                                  "layout-explorer-open",
                                  "layout-explorer-closed",
                                  "layout-operation-open",
                                  "layout-operation-closed",
                                  "layout-inspector-open",
                                  "layout-inspector-closed"}) {
    REQUIRE(small_icons.contains(semantic_id));
  }
}

TEST_CASE("the complete source UI asset bundle loads and rasterizes") {
  const std::filesystem::path asset_root =
      std::filesystem::path(FANCY_UI_TEST_SOURCE_ROOT) / "assets" / "ui";
  REQUIRE(std::filesystem::is_regular_file(asset_root / "fonts" / "OFL.txt"));

  for (const float scale : std::array{1.0f, 1.25f, 1.5f, 2.0f}) {
    ImGui::CreateContext();
    fancy_ui::steppenface::AssetLoadReport report;
    float effective_body_font_height = 0.0f;
    float frame_height = 0.0f;
    float control_height = 0.0f;
    {
      fancy_ui::steppenface::ApplicationUi ui;
      report = ui.Initialize(asset_root, EnvironmentAt(scale));
      const fancy_ui::LayoutMetrics metrics = fancy_ui::CurrentLayoutMetrics();
      effective_body_font_height = ImGui::GetIO().FontDefault->LegacySize;
      frame_height =
          effective_body_font_height + ImGui::GetStyle().FramePadding.y * 2.0f;
      control_height = metrics.geometry.control_height;
    }
    ImGui::DestroyContext();

    INFO("UI scale: " << scale);
    for (const std::string &message : report.messages) {
      INFO(message);
    }
    REQUIRE(report.ok());
    REQUIRE_FALSE(report.used_fallback_font);
    REQUIRE(effective_body_font_height ==
            Catch::Approx((40.0f / 3.0f) * scale * 1362.0f / 1000.0f));
    REQUIRE(frame_height == Catch::Approx(control_height));
  }
}
