#include "fancy_ui/fancy_ui.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <imgui.h>

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
