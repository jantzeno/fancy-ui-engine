#include "gallery_settings_model.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace fancy_ui;
using namespace fancy_ui::gallery;

TEST_CASE("settings gallery defaults match the five-section contract") {
  const SettingsGalleryState state = DefaultSettingsGalleryState();

  REQUIRE(kSettingsSectionCount == 5);
  REQUIRE(state.active_section == SettingsSection::General);
  REQUIRE_FALSE(state.dirty);
  REQUIRE(state.errors.empty());
  REQUIRE(state.draft.general.default_open_directory == "~/Documents");
  REQUIRE(state.draft.general.default_export_directory == "~/Exports");
  REQUIRE(state.draft.machines.profiles.size() == 2);
  REQUIRE(state.draft.machines.selected_id == "router-4x8");
  REQUIRE(SelectedMachine(state)->name == "4 × 8 Router");
}

TEST_CASE("settings apply and discard own one staged preference transaction") {
  SettingsGalleryState state = DefaultSettingsGalleryState();
  state.draft.general.default_export_directory = "~/Output";
  state.draft.general.diagnostics_enabled = true;
  RefreshSettingsDerivedState(state);

  REQUIRE(state.dirty);
  REQUIRE(ApplySettings(state));
  REQUIRE_FALSE(state.dirty);
  REQUIRE(state.applied.general.diagnostics_enabled);

  state.draft.general.default_export_directory.clear();
  RefreshSettingsDerivedState(state);
  REQUIRE_FALSE(ApplySettings(state));
  REQUIRE(state.errors.contains("general.default_export_directory"));

  DiscardSettings(state);
  REQUIRE_FALSE(state.dirty);
  REQUIRE(state.errors.empty());
  REQUIRE(state.draft.general.default_export_directory == "~/Output");
}

TEST_CASE("settings theme preview resolves system and rolls back on discard") {
  SettingsGalleryState state =
      DefaultSettingsGalleryState(ResolvedTheme::Light);
  REQUIRE(ResolveSettingsTheme(state.draft.appearance.theme,
                               state.system_theme) == ResolvedTheme::Light);

  state.draft.appearance.theme = SettingsThemeChoice::Dark;
  RefreshSettingsDerivedState(state);
  REQUIRE(state.dirty);
  REQUIRE(ResolveSettingsTheme(state.draft.appearance.theme,
                               state.system_theme) == ResolvedTheme::Dark);

  DiscardSettings(state);
  REQUIRE(state.draft.appearance.theme == SettingsThemeChoice::System);
}

TEST_CASE("machine profiles derive usable size and enforce lifecycle rules") {
  SettingsGalleryState state = DefaultSettingsGalleryState();
  const UsableBedSize usable = UsableSize(*SelectedMachine(state));
  REQUIRE(usable.width_mm == Catch::Approx(2420.0));
  REQUIRE(usable.height_mm == Catch::Approx(1200.0));
  REQUIRE_FALSE(CanRemoveSelectedMachine(state));

  state.draft.machines.selected_id = "laser-900";
  REQUIRE(CanRemoveSelectedMachine(state));
  DuplicateSelectedMachine(state);
  REQUIRE(state.draft.machines.profiles.size() == 3);
  REQUIRE(SelectedMachine(state)->name == "900 mm Laser copy");
  REQUIRE_FALSE(SelectedMachine(state)->is_default);

  SetSelectedMachineDefault(state);
  REQUIRE(SelectedMachine(state)->is_default);
  REQUIRE_FALSE(CanRemoveSelectedMachine(state));

  state.draft.machines.selected_id = "laser-900";
  REQUIRE(RemoveSelectedMachine(state));
  REQUIRE(state.draft.machines.profiles.size() == 2);
}

TEST_CASE(
    "machine validation reports unique names and impossible usable beds") {
  SettingsGalleryState state = DefaultSettingsGalleryState();
  MachineProfile &laser = state.draft.machines.profiles[1];
  laser.name = "4 × 8 Router";
  laser.edge_insets_mm.left = 500.0;
  laser.edge_insets_mm.right = 500.0;
  RefreshSettingsDerivedState(state);

  REQUIRE(state.errors.contains("machines.laser-900.name"));
  REQUIRE(state.errors.contains("machines.laser-900.usable_width"));
}

TEST_CASE("license actions are immediate and do not dirty preferences") {
  SettingsGalleryState state = DefaultSettingsGalleryState();
  state.license.key = "SHORT";
  REQUIRE_FALSE(ActivateLicense(state));
  REQUIRE(state.license.status == LicenseStatus::Error);
  REQUIRE_FALSE(state.dirty);

  state.license.key = "ABCD-EFGH-IJKL-MNOP";
  REQUIRE(ActivateLicense(state));
  REQUIRE(state.license.status == LicenseStatus::Active);
  REQUIRE(state.license.key.empty());
  REQUIRE_FALSE(state.dirty);

  DeactivateLicense(state);
  REQUIRE(state.license.status == LicenseStatus::Inactive);
}
