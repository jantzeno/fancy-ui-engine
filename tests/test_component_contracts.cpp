#include "fancy_ui/fancy_ui.hpp"
#include "fancy_ui/steppenface/ui_assets.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <imgui.h>

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

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

TEST_CASE("theme scale clamps and scales shared interaction metrics") {
  ImGui::CreateContext();

  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark, 3.0f);
  REQUIRE(fancy_ui::CurrentUiScale() == Catch::Approx(2.0f));
  REQUIRE(ImGui::GetStyle().FramePadding.x == Catch::Approx(24.0f));
  REQUIRE(ImGui::GetStyle().ItemSpacing.y == Catch::Approx(16.0f));
  REQUIRE(ImGui::GetStyle().DisabledAlpha == Catch::Approx(1.0f));

  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Light, 0.5f);
  REQUIRE(fancy_ui::CurrentUiScale() == Catch::Approx(0.75f));
  REQUIRE(fancy_ui::Scale(32.0f) == Catch::Approx(24.0f));
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
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark, 2.0f);

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

  for (const char *semantic_id : {"model", "bed", "objects", "grain", "search",
                                  "compact", "diagnostics"}) {
    REQUIRE(rail_icons.contains(semantic_id));
  }
  for (const char *semantic_id :
       {"information", "success", "alert", "failure", "busy", "visibility",
        "visibility-off", "more", "orbit-locked", "orbit-unlocked"}) {
    REQUIRE(small_icons.contains(semantic_id));
  }
}

TEST_CASE("the complete source UI asset bundle loads and rasterizes") {
  ImGui::CreateContext();
  fancy_ui::steppenface::ApplicationUi ui;
  const std::filesystem::path asset_root =
      std::filesystem::path(FANCY_UI_TEST_SOURCE_ROOT) / "assets" / "ui";

  const fancy_ui::steppenface::AssetLoadReport report =
      ui.Initialize(asset_root, 1.25f);
  for (const std::string &message : report.messages) {
    INFO(message);
  }
  REQUIRE(report.ok());
  REQUIRE_FALSE(report.used_fallback_font);
  ImGui::DestroyContext();
}
