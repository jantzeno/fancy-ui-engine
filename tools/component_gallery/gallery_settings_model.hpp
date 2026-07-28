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
  SettingsPreferences applied;
  SettingsPreferences draft;
  std::map<std::string, std::string> errors;
  LicenseState license;
  bool dirty = false;
  bool window_open = true;
  bool discard_confirmation_open = false;
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
    if (name.empty()) {
      errors[prefix + "name"] = "Enter a machine name.";
    } else if (++names[name] > 1) {
      errors[prefix + "name"] = "Machine names must be unique.";
    }
    if (!(profile.bed_width_mm > 0.0)) {
      errors[prefix + "bed_width_mm"] = "Bed width must be greater than zero.";
    }
    if (!(profile.bed_height_mm > 0.0)) {
      errors[prefix + "bed_height_mm"] =
          "Bed height must be greater than zero.";
    }
    const std::array edge_values{
        std::pair{"top", profile.edge_insets_mm.top},
        std::pair{"right", profile.edge_insets_mm.right},
        std::pair{"bottom", profile.edge_insets_mm.bottom},
        std::pair{"left", profile.edge_insets_mm.left},
    };
    for (const auto &[edge, value] : edge_values) {
      if (!(value >= 0.0)) {
        errors[prefix + "edge_insets_mm." + edge] =
            "Edge inset cannot be negative.";
      }
    }
    const UsableBedSize usable = UsableSize(profile);
    if (!(usable.width_mm > 0.0)) {
      errors[prefix + "usable_width"] =
          "Left and right insets must leave a usable bed width.";
    }
    if (!(usable.height_mm > 0.0)) {
      errors[prefix + "usable_height"] =
          "Top and bottom insets must leave a usable bed height.";
    }
    if (profile.material_thickness_mm.has_value() &&
        !(*profile.material_thickness_mm > 0.0)) {
      errors[prefix + "material_thickness_mm"] =
          "Material thickness must be blank or greater than zero.";
    }
    default_count += profile.is_default ? 1U : 0U;
  }
  if (default_count != 1) {
    errors["machines.default"] = "Choose exactly one default machine.";
  }
  return errors;
}

inline void RefreshSettingsDerivedState(SettingsGalleryState &state) {
  state.dirty = state.applied != state.draft;
  state.errors = ValidateSettings(state.draft);
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

inline void AddMachine(SettingsGalleryState &state) {
  const std::string id = NextMachineId(state.draft.machines.profiles);
  state.draft.machines.profiles.push_back({
      .id = id,
      .name =
          "Machine " + std::to_string(state.draft.machines.profiles.size() + 1),
      .bed_width_mm = 1000.0,
      .bed_height_mm = 600.0,
  });
  state.draft.machines.selected_id = id;
  RefreshSettingsDerivedState(state);
}

inline void DuplicateSelectedMachine(SettingsGalleryState &state) {
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
  return profile != nullptr && !profile->is_default &&
         state.draft.machines.profiles.size() > 1;
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
  if (!state.errors.empty()) {
    return false;
  }
  state.applied = state.draft;
  RefreshSettingsDerivedState(state);
  return true;
}

inline void DiscardSettings(SettingsGalleryState &state) {
  state.draft = state.applied;
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
