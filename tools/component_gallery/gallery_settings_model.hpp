#pragma once

#include "fancy_ui/theme.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fancy_ui::gallery {

enum class SettingsSection {
  General,
  Appearance,
  Machines,
  License,
  Legal,
};

enum class SettingsThemeChoice {
  System,
  Light,
  Dark,
};

enum class MachineOrigin {
  BottomLeft,
  BottomRight,
  TopLeft,
  TopRight,
};

enum class MachineSettingsTab {
  Profiles,
  BedArea,
  Information,
};

enum class MachineEditorMode {
  New,
  Edit,
};

enum class LicenseStatus {
  Inactive,
  Error,
  Active,
};

inline constexpr std::size_t kSettingsSectionCount = 5;

struct GeneralSettings {
  std::string default_open_directory = "~/Documents";
  std::string default_export_directory = "~/Exports";
  bool default_canvas_view = false;
  bool diagnostics_enabled = false;

  [[nodiscard]] bool operator==(const GeneralSettings &) const = default;
};

struct AppearanceSettings {
  SettingsThemeChoice theme = SettingsThemeChoice::System;

  [[nodiscard]] bool operator==(const AppearanceSettings &) const = default;
};

struct EdgeInsets {
  double top = 0.0;
  double right = 0.0;
  double bottom = 0.0;
  double left = 0.0;

  [[nodiscard]] bool operator==(const EdgeInsets &) const = default;
};

struct MachineProfile {
  std::string id;
  std::string name;
  bool is_default = false;
  double bed_width_mm = 0.0;
  double bed_height_mm = 0.0;
  EdgeInsets edge_insets_mm;
  MachineOrigin origin = MachineOrigin::BottomLeft;
  std::optional<double> material_thickness_mm;

  [[nodiscard]] bool operator==(const MachineProfile &) const = default;
};

struct MachinePreset {
  std::string_view id;
  std::string_view manufacturer;
  std::string_view model;
  std::string_view variant;
  double bed_width_mm;
  double bed_height_mm;
  MachineOrigin origin;
  bool requires_origin_confirmation;
};

struct MachineEditorState {
  MachineEditorMode mode = MachineEditorMode::New;
  std::optional<std::string> source_id;
  MachineProfile baseline;
  MachineProfile draft;
  std::map<std::string, std::string> errors;
  bool dirty = false;
  bool preset_picker_open = false;
  std::string preset_manufacturer = "Creality";
  std::optional<std::string> applied_preset_id;
  bool origin_confirmation_required = false;
  bool origin_confirmed = true;
};

struct MachineSettings {
  std::string selected_id;
  std::vector<MachineProfile> profiles;

  [[nodiscard]] bool operator==(const MachineSettings &) const = default;
};

struct SettingsPreferences {
  GeneralSettings general;
  AppearanceSettings appearance;
  MachineSettings machines;

  [[nodiscard]] bool operator==(const SettingsPreferences &) const = default;
};

struct LicenseState {
  LicenseStatus status = LicenseStatus::Inactive;
  std::string key;
  std::string masked_key = "•••• •••• •••• 7K2Q";
  std::string owner = "SonderMill Studio";
  std::string error;

  [[nodiscard]] bool operator==(const LicenseState &) const = default;
};

struct SettingsGalleryState {
  SettingsSection active_section = SettingsSection::General;
  MachineSettingsTab active_machine_tab = MachineSettingsTab::Profiles;
  std::optional<MachineEditorState> machine_editor;
  SettingsPreferences applied;
  SettingsPreferences draft;
  std::map<std::string, std::string> errors;
  LicenseState license;
  bool dirty = false;
  bool window_open = true;
  bool discard_confirmation_open = false;
  bool remove_confirmation_open = false;
  bool request_machine_confirmation_scroll = false;
  bool request_window_focus = true;
  ResolvedTheme system_theme = ResolvedTheme::Dark;
};

struct UsableBedSize {
  double width_mm = 0.0;
  double height_mm = 0.0;

  [[nodiscard]] bool operator==(const UsableBedSize &) const = default;
};

[[nodiscard]] inline SettingsPreferences DefaultSettingsPreferences() {
  SettingsPreferences settings;
  settings.machines.selected_id = "router-4x8";
  settings.machines.profiles = {
      {
          .id = "router-4x8",
          .name = "4 × 8 Router",
          .is_default = true,
          .bed_width_mm = 2440.0,
          .bed_height_mm = 1220.0,
          .edge_insets_mm =
              {
                  .top = 10.0,
                  .right = 10.0,
                  .bottom = 10.0,
                  .left = 10.0,
              },
          .origin = MachineOrigin::BottomLeft,
          .material_thickness_mm = 18.0,
      },
      {
          .id = "laser-900",
          .name = "900 mm Laser",
          .bed_width_mm = 900.0,
          .bed_height_mm = 600.0,
          .edge_insets_mm =
              {
                  .top = 5.0,
                  .right = 5.0,
                  .bottom = 5.0,
                  .left = 5.0,
              },
          .origin = MachineOrigin::TopLeft,
      },
  };
  return settings;
}

[[nodiscard]] inline UsableBedSize UsableSize(const MachineProfile &profile) {
  return {
      .width_mm = profile.bed_width_mm - profile.edge_insets_mm.left -
                  profile.edge_insets_mm.right,
      .height_mm = profile.bed_height_mm - profile.edge_insets_mm.top -
                   profile.edge_insets_mm.bottom,
  };
}

inline constexpr std::array<MachinePreset, 25> kMachinePresets{{
    {"atomstack-a10-pro-v2-10w", "AtomStack", "A10 Pro V2", "10W", 400.0, 365.0,
     MachineOrigin::BottomLeft, true},
    {"atomstack-a20-pro-v2-20w", "AtomStack", "A20 Pro V2", "20W", 400.0, 365.0,
     MachineOrigin::BottomLeft, true},
    {"creality-falcon-cr-5w-10w", "Creality", "Falcon CR", "5W/10W", 400.0,
     415.0, MachineOrigin::BottomLeft, true},
    {"creality-falcon2-22w-40w", "Creality", "Falcon2", "22W/40W", 400.0, 415.0,
     MachineOrigin::BottomLeft, true},
    {"creality-falcon2-pro-22w-40w", "Creality", "Falcon2 Pro", "22W/40W",
     400.0, 415.0, MachineOrigin::BottomLeft, true},
    {"creality-falcon2-pro-60w", "Creality", "Falcon2 Pro", "60W", 400.0, 400.0,
     MachineOrigin::BottomLeft, true},
    {"creality-falcon2-pro-s-22w-40w", "Creality", "Falcon2 Pro S", "22W/40W",
     355.0, 390.0, MachineOrigin::BottomLeft, true},
    {"glowforge-aura-standard", "Glowforge", "Aura", "standard", 304.8, 304.8,
     MachineOrigin::TopLeft, true},
    {"omtech-2028-maker-35-pronto-35-20x28-class", "OMTech",
     "2028 / Maker 35 / Pronto 35", "20x28 class", 711.2, 508.0,
     MachineOrigin::TopRight, true},
    {"omtech-2435-pronto-40-24x35-class", "OMTech", "2435 / Pronto 40",
     "24x35 class", 889.0, 609.6, MachineOrigin::TopRight, true},
    {"omtech-mf1220-50w-cabinet", "OMTech", "MF1220", "50W cabinet", 508.0,
     304.8, MachineOrigin::TopRight, true},
    {"omtech-mf1624-55w-cabinet", "OMTech", "MF1624", "55W cabinet", 609.6,
     406.4, MachineOrigin::TopRight, true},
    {"omtech-polar-desktop-co2", "OMTech", "Polar", "desktop CO2", 510.0, 300.0,
     MachineOrigin::TopRight, false},
    {"ortur-laser-master-3-10w", "Ortur", "Laser Master 3", "10W", 400.0, 400.0,
     MachineOrigin::BottomLeft, true},
    {"ortur-laser-master-3-20w-40w", "Ortur", "Laser Master 3", "20W/40W",
     400.0, 380.0, MachineOrigin::BottomLeft, true},
    {"thunder-laser-bolt-standard", "Thunder Laser", "Bolt", "standard", 508.0,
     304.8, MachineOrigin::TopLeft, false},
    {"thunder-laser-bolt-plus-24-standard", "Thunder Laser", "Bolt Plus 24",
     "standard", 609.6, 406.4, MachineOrigin::TopLeft, false},
    {"thunder-laser-bolt-pro-36-standard", "Thunder Laser", "Bolt Pro 36",
     "standard", 914.4, 609.6, MachineOrigin::TopLeft, false},
    {"xtool-d1-20w-ir-configuration", "xTool", "D1", "20W/IR configuration",
     430.0, 390.0, MachineOrigin::TopLeft, false},
    {"xtool-d1-5w-10w", "xTool", "D1", "5W/10W", 430.0, 400.0,
     MachineOrigin::TopLeft, false},
    {"xtool-m1-base-removed", "xTool", "M1", "base removed", 385.0, 270.0,
     MachineOrigin::TopLeft, true},
    {"xtool-m1-laser-flat", "xTool", "M1", "laser flat", 385.0, 300.0,
     MachineOrigin::TopLeft, true},
    {"xtool-p2-p2s-honeycomb-panel", "xTool", "P2/P2S", "honeycomb panel",
     556.0, 280.0, MachineOrigin::TopLeft, false},
    {"xtool-s1-20w", "xTool", "S1", "20W", 498.0, 330.0, MachineOrigin::TopLeft,
     true},
    {"xtool-s1-40w", "xTool", "S1", "40W", 498.0, 319.0, MachineOrigin::TopLeft,
     true},
}};

inline constexpr std::array<std::string_view, 7> kMachinePresetManufacturers{
    "AtomStack", "Creality",      "Glowforge", "OMTech",
    "Ortur",     "Thunder Laser", "xTool"};

[[nodiscard]] inline MachineProfile
MachineProfileFromPreset(const MachinePreset &preset) {
  return {
      .name =
          std::string(preset.manufacturer) + " " + std::string(preset.model),
      .bed_width_mm = preset.bed_width_mm,
      .bed_height_mm = preset.bed_height_mm,
      .origin = preset.origin,
  };
}

[[nodiscard]] inline std::string TrimmedLower(std::string value) {
  const auto first =
      std::find_if_not(value.begin(), value.end(), [](const unsigned char c) {
        return std::isspace(c) != 0;
      });
  const auto last =
      std::find_if_not(value.rbegin(), value.rend(), [](const unsigned char c) {
        return std::isspace(c) != 0;
      }).base();
  if (first >= last) {
    return {};
  }
  value = std::string(first, last);
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

[[nodiscard]] inline std::map<std::string, std::string>
ValidateMachineProfile(const MachineProfile &profile) {
  std::map<std::string, std::string> errors;
  if (TrimmedLower(profile.name).empty()) {
    errors["name"] = "Enter a machine name.";
  }
  if (!(profile.bed_width_mm > 0.0)) {
    errors["bed_width_mm"] = "Bed width must be greater than zero.";
  }
  if (!(profile.bed_height_mm > 0.0)) {
    errors["bed_height_mm"] = "Bed height must be greater than zero.";
  }
  const std::array edge_values{
      std::pair{"top", profile.edge_insets_mm.top},
      std::pair{"right", profile.edge_insets_mm.right},
      std::pair{"bottom", profile.edge_insets_mm.bottom},
      std::pair{"left", profile.edge_insets_mm.left},
  };
  for (const auto &[edge, value] : edge_values) {
    if (!(value >= 0.0)) {
      errors["edge_insets_mm." + std::string(edge)] =
          "Edge inset cannot be negative.";
    }
  }
  const UsableBedSize usable = UsableSize(profile);
  if (!(usable.width_mm > 0.0)) {
    errors["usable_width"] =
        "Left and right insets must leave a usable bed width.";
  }
  if (!(usable.height_mm > 0.0)) {
    errors["usable_height"] =
        "Top and bottom insets must leave a usable bed height.";
  }
  if (profile.material_thickness_mm.has_value() &&
      !(*profile.material_thickness_mm > 0.0)) {
    errors["material_thickness_mm"] =
        "Material thickness must be blank or greater than zero.";
  }
  return errors;
}

[[nodiscard]] inline std::map<std::string, std::string>
ValidateSettings(const SettingsPreferences &settings) {
  std::map<std::string, std::string> errors;
  if (TrimmedLower(settings.general.default_open_directory).empty()) {
    errors["general.default_open_directory"] =
        "Choose a default open/import directory.";
  }
  if (TrimmedLower(settings.general.default_export_directory).empty()) {
    errors["general.default_export_directory"] =
        "Choose a default export directory.";
  }

  std::map<std::string, std::size_t> names;
  std::size_t default_count = 0;
  for (const MachineProfile &profile : settings.machines.profiles) {
    const std::string prefix = "machines." + profile.id + ".";
    const std::string name = TrimmedLower(profile.name);
    for (const auto &[field, message] : ValidateMachineProfile(profile)) {
      errors[prefix + field] = message;
    }
    if (!name.empty() && ++names[name] > 1) {
      errors[prefix + "name"] = "Machine names must be unique.";
    }
    default_count += profile.is_default ? 1U : 0U;
  }
  if (default_count != 1) {
    errors["machines.default"] = "Choose exactly one default machine.";
  }
  return errors;
}

[[nodiscard]] inline std::map<std::string, std::string>
ValidateMachineEditor(const MachineEditorState &editor,
                      const std::vector<MachineProfile> &profiles) {
  auto errors = ValidateMachineProfile(editor.draft);
  const std::string name = TrimmedLower(editor.draft.name);
  if (!name.empty() &&
      std::ranges::any_of(profiles,
                          [&editor, &name](const MachineProfile &item) {
                            return item.id != editor.source_id.value_or("") &&
                                   TrimmedLower(item.name) == name;
                          })) {
    errors["name"] = "Machine names must be unique.";
  }
  if (editor.origin_confirmation_required && !editor.origin_confirmed) {
    errors["origin_confirmation"] =
        "Confirm the suggested origin or choose another corner.";
  }
  return errors;
}

inline void RefreshSettingsDerivedState(SettingsGalleryState &state) {
  state.dirty = state.applied != state.draft;
  state.errors = ValidateSettings(state.draft);
  if (state.machine_editor.has_value()) {
    state.machine_editor->dirty =
        state.machine_editor->baseline != state.machine_editor->draft;
    state.machine_editor->errors = ValidateMachineEditor(
        *state.machine_editor, state.draft.machines.profiles);
  }
}

[[nodiscard]] inline SettingsGalleryState DefaultSettingsGalleryState(
    const ResolvedTheme system_theme = ResolvedTheme::Dark) {
  SettingsGalleryState state;
  state.applied = DefaultSettingsPreferences();
  state.draft = state.applied;
  state.system_theme = system_theme;
  RefreshSettingsDerivedState(state);
  return state;
}

[[nodiscard]] inline ResolvedTheme
ResolveSettingsTheme(const SettingsThemeChoice theme,
                     const ResolvedTheme system_theme) {
  switch (theme) {
  case SettingsThemeChoice::System:
    return system_theme;
  case SettingsThemeChoice::Light:
    return ResolvedTheme::Light;
  case SettingsThemeChoice::Dark:
    return ResolvedTheme::Dark;
  }
  return ResolvedTheme::Dark;
}

[[nodiscard]] inline MachineProfile *
SelectedMachine(SettingsGalleryState &state) {
  const auto found =
      std::find_if(state.draft.machines.profiles.begin(),
                   state.draft.machines.profiles.end(),
                   [&state](const MachineProfile &item) {
                     return item.id == state.draft.machines.selected_id;
                   });
  return found == state.draft.machines.profiles.end() ? nullptr : &*found;
}

[[nodiscard]] inline const MachineProfile *
SelectedMachine(const SettingsGalleryState &state) {
  const auto found =
      std::find_if(state.draft.machines.profiles.begin(),
                   state.draft.machines.profiles.end(),
                   [&state](const MachineProfile &item) {
                     return item.id == state.draft.machines.selected_id;
                   });
  return found == state.draft.machines.profiles.end() ? nullptr : &*found;
}

[[nodiscard]] inline std::string
NextMachineId(const std::vector<MachineProfile> &profiles) {
  std::size_t index = profiles.size() + 1;
  while (std::ranges::any_of(profiles, [index](const MachineProfile &profile) {
    return profile.id == "machine-" + std::to_string(index);
  })) {
    ++index;
  }
  return "machine-" + std::to_string(index);
}

inline void BeginNewMachine(SettingsGalleryState &state) {
  MachineProfile draft;
  state.machine_editor = MachineEditorState{
      .mode = MachineEditorMode::New,
      .baseline = draft,
      .draft = draft,
  };
  state.active_machine_tab = MachineSettingsTab::Information;
  RefreshSettingsDerivedState(state);
}

[[nodiscard]] inline bool BeginEditMachine(SettingsGalleryState &state,
                                           const std::string_view id) {
  const auto found =
      std::ranges::find(state.draft.machines.profiles, id, &MachineProfile::id);
  if (found == state.draft.machines.profiles.end()) {
    return false;
  }
  state.machine_editor = MachineEditorState{
      .mode = MachineEditorMode::Edit,
      .source_id = found->id,
      .baseline = *found,
      .draft = *found,
      .preset_manufacturer = {},
  };
  state.active_machine_tab = MachineSettingsTab::Information;
  RefreshSettingsDerivedState(state);
  return true;
}

inline void CancelMachineEditor(SettingsGalleryState &state) {
  state.machine_editor.reset();
  state.active_machine_tab = MachineSettingsTab::Profiles;
  RefreshSettingsDerivedState(state);
}

[[nodiscard]] inline bool OpenMachinePresetPicker(SettingsGalleryState &state) {
  if (!state.machine_editor.has_value() ||
      state.machine_editor->mode != MachineEditorMode::New) {
    return false;
  }
  state.machine_editor->preset_picker_open = true;
  return true;
}

inline void CloseMachinePresetPicker(SettingsGalleryState &state) {
  if (state.machine_editor.has_value()) {
    state.machine_editor->preset_picker_open = false;
  }
}

[[nodiscard]] inline bool
SelectMachinePresetManufacturer(SettingsGalleryState &state,
                                const std::string_view manufacturer) {
  if (!state.machine_editor.has_value() ||
      state.machine_editor->mode != MachineEditorMode::New ||
      !std::ranges::any_of(kMachinePresets,
                           [manufacturer](const MachinePreset &preset) {
                             return preset.manufacturer == manufacturer;
                           })) {
    return false;
  }
  state.machine_editor->preset_manufacturer = manufacturer;
  return true;
}

[[nodiscard]] inline bool ApplyMachinePreset(SettingsGalleryState &state,
                                             const std::string_view id) {
  if (!state.machine_editor.has_value() ||
      state.machine_editor->mode != MachineEditorMode::New ||
      !state.machine_editor->preset_picker_open) {
    return false;
  }
  const auto found = std::ranges::find(kMachinePresets, id, &MachinePreset::id);
  if (found == kMachinePresets.end()) {
    return false;
  }
  MachineEditorState &editor = *state.machine_editor;
  editor.draft = MachineProfileFromPreset(*found);
  editor.preset_picker_open = false;
  editor.preset_manufacturer = found->manufacturer;
  editor.applied_preset_id = std::string(found->id);
  editor.origin_confirmation_required = found->requires_origin_confirmation;
  editor.origin_confirmed = !found->requires_origin_confirmation;
  RefreshSettingsDerivedState(state);
  return true;
}

inline void SetMachineEditorOrigin(SettingsGalleryState &state,
                                   const MachineOrigin origin) {
  if (!state.machine_editor.has_value()) {
    return;
  }
  state.machine_editor->draft.origin = origin;
  state.machine_editor->origin_confirmed = true;
  RefreshSettingsDerivedState(state);
}

inline void ConfirmMachinePresetOrigin(SettingsGalleryState &state) {
  if (!state.machine_editor.has_value() ||
      !state.machine_editor->origin_confirmation_required) {
    return;
  }
  state.machine_editor->origin_confirmed = true;
  RefreshSettingsDerivedState(state);
}

[[nodiscard]] inline bool SaveMachineEditor(SettingsGalleryState &state) {
  if (!state.machine_editor.has_value()) {
    return false;
  }
  RefreshSettingsDerivedState(state);
  if (!state.machine_editor->errors.empty()) {
    return false;
  }
  const MachineEditorState editor = *state.machine_editor;
  MachineProfile saved = editor.draft;
  if (editor.mode == MachineEditorMode::New) {
    saved.id = NextMachineId(state.draft.machines.profiles);
    state.draft.machines.profiles.push_back(saved);
  } else {
    const auto found =
        std::ranges::find(state.draft.machines.profiles,
                          editor.source_id.value_or(""), &MachineProfile::id);
    if (found == state.draft.machines.profiles.end()) {
      return false;
    }
    saved.id = found->id;
    *found = saved;
  }
  if (saved.is_default) {
    for (MachineProfile &profile : state.draft.machines.profiles) {
      profile.is_default = profile.id == saved.id;
    }
  }
  state.draft.machines.selected_id = saved.id;
  state.machine_editor.reset();
  state.active_machine_tab = MachineSettingsTab::Profiles;
  RefreshSettingsDerivedState(state);
  return true;
}

inline void DuplicateSelectedMachine(SettingsGalleryState &state) {
  if (state.machine_editor.has_value()) {
    return;
  }
  const MachineProfile *source = SelectedMachine(state);
  if (source == nullptr) {
    return;
  }
  MachineProfile duplicate = *source;
  duplicate.id = NextMachineId(state.draft.machines.profiles);
  duplicate.name += " copy";
  duplicate.is_default = false;
  state.draft.machines.profiles.push_back(duplicate);
  state.draft.machines.selected_id = duplicate.id;
  RefreshSettingsDerivedState(state);
}

inline void SetSelectedMachineDefault(SettingsGalleryState &state) {
  for (MachineProfile &profile : state.draft.machines.profiles) {
    profile.is_default = profile.id == state.draft.machines.selected_id;
  }
  RefreshSettingsDerivedState(state);
}

[[nodiscard]] inline bool
CanRemoveSelectedMachine(const SettingsGalleryState &state) {
  const MachineProfile *profile = SelectedMachine(state);
  return !state.machine_editor.has_value() && profile != nullptr &&
         !profile->is_default && state.draft.machines.profiles.size() > 1;
}

inline bool RemoveSelectedMachine(SettingsGalleryState &state) {
  if (!CanRemoveSelectedMachine(state)) {
    return false;
  }
  const std::string selected = state.draft.machines.selected_id;
  std::erase_if(state.draft.machines.profiles,
                [&selected](const MachineProfile &profile) {
                  return profile.id == selected;
                });
  state.draft.machines.selected_id = state.draft.machines.profiles.front().id;
  RefreshSettingsDerivedState(state);
  return true;
}

[[nodiscard]] inline bool ApplySettings(SettingsGalleryState &state) {
  RefreshSettingsDerivedState(state);
  if (state.machine_editor.has_value() || !state.errors.empty()) {
    return false;
  }
  state.applied = state.draft;
  RefreshSettingsDerivedState(state);
  return true;
}

inline void DiscardSettings(SettingsGalleryState &state) {
  state.draft = state.applied;
  state.machine_editor.reset();
  state.active_machine_tab = MachineSettingsTab::Profiles;
  RefreshSettingsDerivedState(state);
}

[[nodiscard]] inline bool ActivateLicense(SettingsGalleryState &state) {
  const std::size_t characters = static_cast<std::size_t>(std::count_if(
      state.license.key.begin(), state.license.key.end(),
      [](const unsigned char c) { return std::isalnum(c) != 0; }));
  if (characters < 12) {
    state.license.status = LicenseStatus::Error;
    state.license.error = "Enter the complete product key.";
    return false;
  }
  state.license.status = LicenseStatus::Active;
  state.license.error.clear();
  state.license.key.clear();
  return true;
}

inline void DeactivateLicense(SettingsGalleryState &state) {
  state.license.status = LicenseStatus::Inactive;
  state.license.error.clear();
}

} // namespace fancy_ui::gallery
