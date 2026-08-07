#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <string_view>

namespace fancy_ui::gallery {

namespace {

Validation ValidationFor(const SettingsGalleryState &state,
                         const std::string &path) {
  const auto found = state.errors.find(path);
  if (found == state.errors.end()) {
    return {};
  }
  return {
      .invalid = true,
      .message = found->second,
  };
}

void DrawPageHeader(detail::UiAssetAtlas &assets, const char *title,
                    const char *description) {
  if (assets.bold_font() != nullptr) {
    ImGui::PushFont(assets.bold_font());
  }
  ImGui::TextUnformatted(title);
  if (assets.bold_font() != nullptr) {
    ImGui::PopFont();
  }
  ImGui::TextWrapped("%s", description);
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

void DrawGeneral(detail::UiAssetAtlas &assets, GalleryState &gallery) {
  SettingsGalleryState &state = gallery.settings;
  GeneralSettings &general = state.draft.general;
  DrawPageHeader(
      assets, "General",
      "Choose file locations, the startup workspace, and optional diagnostic "
      "tools.");

  ImGui::SeparatorText("Files");
  {
    if (ImGui::BeginTable("##directory-rows", 2,
                          ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                              Scale(96.0f));
      ImGui::TableNextColumn();
      const TextInputResult open = TextInput({
          .id = "default-open-directory",
          .label = "Default open/import directory",
          .value = general.default_open_directory,
          .validation = ValidationFor(state, "general.default_open_directory"),
      });
      if (open.changed) {
        general.default_open_directory = open.value;
        RefreshSettingsDerivedState(state);
      }
      ImGui::TableNextColumn();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                           ImGui::GetTextLineHeightWithSpacing());
      if (Button({.id = "browse-open",
                  .label = "Browse…",
                  .variant = ButtonVariant::Secondary,
                  .size = {.x = -1.0f, .y = 32.0f}})
              .activated) {
        general.default_open_directory = "~/Documents/CNC";
        RefreshSettingsDerivedState(state);
      }

      ImGui::TableNextColumn();
      const TextInputResult export_path = TextInput({
          .id = "default-export-directory",
          .label = "Default export directory",
          .value = general.default_export_directory,
          .validation =
              ValidationFor(state, "general.default_export_directory"),
      });
      if (export_path.changed) {
        general.default_export_directory = export_path.value;
        RefreshSettingsDerivedState(state);
      }
      ImGui::TableNextColumn();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                           ImGui::GetTextLineHeightWithSpacing());
      if (Button({.id = "browse-export",
                  .label = "Browse…",
                  .variant = ButtonVariant::Secondary,
                  .size = {.x = -1.0f, .y = 32.0f}})
              .activated) {
        general.default_export_directory = "~/Exports/CNC";
        RefreshSettingsDerivedState(state);
      }
      ImGui::EndTable();
    }
  }

  ImGui::SeparatorText("Startup");
  {
    ImGui::TextUnformatted("Default view");
    if (ImGui::RadioButton("3D", !general.default_canvas_view)) {
      general.default_canvas_view = false;
      RefreshSettingsDerivedState(state);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Canvas", general.default_canvas_view)) {
      general.default_canvas_view = true;
      RefreshSettingsDerivedState(state);
    }
    const CheckboxResult diagnostics = Checkbox({
        .id = "diagnostics-enabled",
        .label = "Enable diagnostics",
        .state =
            general.diagnostics_enabled ? ToggleState::On : ToggleState::Off,
    });
    if (diagnostics.changed) {
      general.diagnostics_enabled = diagnostics.state == ToggleState::On;
      RefreshSettingsDerivedState(state);
    }
    ImGui::TextWrapped(
        "Adds Diagnostics to the Canvas activity bar after Apply.");
  }
}

void DrawThemePreview(const ResolvedTheme preview) {
  const SemanticPalette palette = PaletteFor(preview);
  const ImVec4 background(palette.surface.red, palette.surface.green,
                          palette.surface.blue, palette.surface.alpha);
  const ImVec4 border(palette.border_strong.red, palette.border_strong.green,
                      palette.border_strong.blue, palette.border_strong.alpha);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
  ImGui::PushStyleColor(ImGuiCol_Border, border);
  if (ImGui::BeginChild("##settings-theme-preview", ImVec2(0.0f, Scale(112.0f)),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar)) {
    ImGui::TextUnformatted("Export Face");
    ImGui::Separator();
    ImGui::TextUnformatted("Frame plate");
    detail::DrawSecondaryText("1 object selected · Ready to edit");
    StatusText({
        .label = "No geometry issues",
        .status = SemanticStatus::Success,
    });
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(2);
}

void DrawAppearance(detail::UiAssetAtlas &assets, GalleryState &gallery) {
  SettingsGalleryState &state = gallery.settings;
  DrawPageHeader(
      assets, "Appearance",
      "Theme changes preview immediately and remain reversible until Apply.");
  ImGui::TextUnformatted("Theme");
  struct ThemeOption {
    SettingsThemeChoice choice;
    const char *label;
    const char *description;
  };
  static constexpr std::array options{
      ThemeOption{SettingsThemeChoice::System, "System",
                  "Follow the operating-system appearance"},
      ThemeOption{SettingsThemeChoice::Light, "Light",
                  "Use the light semantic palette"},
      ThemeOption{SettingsThemeChoice::Dark, "Dark",
                  "Use the dark semantic palette"},
  };
  if (ImGui::BeginTable("##theme-options", 3,
                        ImGuiTableFlags_SizingStretchSame)) {
    for (const ThemeOption &option : options) {
      ImGui::TableNextColumn();
      ImGui::PushID(option.label);
      if (ImGui::BeginChild("##theme-card", ImVec2(0.0f, Scale(96.0f)),
                            ImGuiChildFlags_Borders |
                                ImGuiChildFlags_AlwaysUseWindowPadding)) {
        if (Button({.id = "select",
                    .label = option.label,
                    .variant = ButtonVariant::Tertiary,
                    .selected = state.draft.appearance.theme == option.choice,
                    .size = {.x = -1.0f, .y = 32.0f}})
                .activated) {
          state.draft.appearance.theme = option.choice;
          RefreshSettingsDerivedState(state);
          gallery.theme =
              ResolveSettingsTheme(option.choice, state.system_theme);
        }
        detail::DrawSecondaryTextWrapped(option.description);
      }
      ImGui::EndChild();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::Spacing();
  ImGui::TextUnformatted("Preview");
  DrawThemePreview(
      ResolveSettingsTheme(state.draft.appearance.theme, state.system_theme));
}

std::size_t OriginIndex(const MachineOrigin origin) {
  switch (origin) {
  case MachineOrigin::BottomLeft:
    return 0;
  case MachineOrigin::BottomRight:
    return 1;
  case MachineOrigin::TopLeft:
    return 2;
  case MachineOrigin::TopRight:
    return 3;
  }
  return 0;
}

MachineOrigin OriginFromIndex(const std::size_t index) {
  static constexpr std::array origins{
      MachineOrigin::BottomLeft,
      MachineOrigin::BottomRight,
      MachineOrigin::TopLeft,
      MachineOrigin::TopRight,
  };
  return origins[std::min(index, origins.size() - 1)];
}

Validation MachineEditorValidation(const SettingsGalleryState &state,
                                   const std::string &path) {
  if (!state.machine_editor.has_value()) {
    return {};
  }
  const auto found = state.machine_editor->errors.find(path);
  return found == state.machine_editor->errors.end()
             ? Validation{}
             : Validation{.invalid = true, .message = found->second};
}

bool DrawMachineEditorNumber(SettingsGalleryState &state, const char *id,
                             const char *label, double &value,
                             const std::string &error_path) {
  const NumericInputResult result = NumericInput({
      .id = id,
      .label = label,
      .unit = "mm",
      .value = value,
      .format = "%.1f",
      .validation = MachineEditorValidation(state, error_path),
  });
  if (result.changed) {
    value = result.value;
    RefreshSettingsDerivedState(state);
  }
  return result.changed;
}

void DrawMachineProfiles(SettingsGalleryState &state) {
  ImGui::TextUnformatted("Machine profiles");
  detail::DrawSecondaryText(
      "Select the profile used by the bed summary and profile actions.");
  ImGui::SameLine(std::max(ImGui::GetCursorPosX(),
                           ImGui::GetWindowWidth() - Scale(136.0f)));
  if (Button({.id = "new-machine",
              .label = "New machine",
              .variant = ButtonVariant::Primary,
              .size = {.x = 120.0f, .y = 32.0f}})
          .activated) {
    BeginNewMachine(state);
  }

  for (const MachineProfile &profile : state.draft.machines.profiles) {
    ImGui::PushID(profile.id.c_str());
    const std::string label =
        profile.name + (profile.is_default ? " · Default" : "");
    if (Button({.id = "select",
                .label = label,
                .variant = ButtonVariant::Tertiary,
                .selected = profile.id == state.draft.machines.selected_id,
                .size = {.x = -1.0f, .y = 36.0f}})
            .activated) {
      state.draft.machines.selected_id = profile.id;
      RefreshSettingsDerivedState(state);
    }
    ImGui::PopID();
  }

  const bool editor_open = state.machine_editor.has_value();
  if (Button({.id = "edit-machine",
              .label = "Edit",
              .availability = {.enabled = !editor_open,
                               .reason =
                                   "Save or cancel Machine Information first"},
              .size = {.x = 80.0f, .y = 32.0f}})
          .activated) {
    static_cast<void>(
        BeginEditMachine(state, state.draft.machines.selected_id));
  }
  ImGui::SameLine();
  if (Button({.id = "duplicate-machine",
              .label = "Duplicate",
              .availability = {.enabled = !editor_open,
                               .reason =
                                   "Save or cancel Machine Information first"},
              .size = {.x = 104.0f, .y = 32.0f}})
          .activated) {
    DuplicateSelectedMachine(state);
  }
  ImGui::SameLine();
  if (Button({.id = "remove-machine",
              .label = "Remove",
              .variant = ButtonVariant::Destructive,
              .availability =
                  {.enabled = CanRemoveSelectedMachine(state),
                   .reason =
                       editor_open
                           ? "Save or cancel Machine Information first"
                           : "The default or final machine cannot be removed"},
              .size = {.x = 88.0f, .y = 32.0f}})
          .activated) {
    ImGui::OpenPopup("Remove machine profile");
  }
  if (state.remove_confirmation_open) {
    ImGui::OpenPopup("Remove machine profile");
    state.remove_confirmation_open = false;
  }
  if (editor_open) {
    detail::DrawSecondaryText(
        "Machine Information is retained until you save or cancel it.");
  }

  if (ImGui::BeginPopupModal("Remove machine profile", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    const MachineProfile *selected = SelectedMachine(state);
    ImGui::TextWrapped("Remove %s from the staged machine profiles?",
                       selected == nullptr ? "this machine"
                                           : selected->name.c_str());
    if (Button({.id = "confirm-remove",
                .label = "Remove",
                .variant = ButtonVariant::Destructive})
            .activated) {
      static_cast<void>(RemoveSelectedMachine(state));
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (Button({.id = "cancel-remove", .label = "Keep machine"}).activated) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawMachineBedArea(SettingsGalleryState &state) {
  const MachineProfile *profile = SelectedMachine(state);
  if (profile == nullptr) {
    return;
  }
  ImGui::TextUnformatted(profile->name.c_str());
  detail::DrawSecondaryText(
      "Read-only geometry derived from the selected staged profile.");
  static_cast<void>(ValueDisplay({
      .id = "physical-bed-size",
      .label = "Physical bed",
      .value = std::format("{:.0f} × {:.0f} mm", profile->bed_width_mm,
                           profile->bed_height_mm),
  }));
  const UsableBedSize usable = UsableSize(*profile);
  static_cast<void>(ValueDisplay({
      .id = "usable-bed-size",
      .label = "Usable bed area",
      .value =
          std::format("{:.0f} × {:.0f} mm", usable.width_mm, usable.height_mm),
  }));
  static_cast<void>(ValueDisplay({
      .id = "edge-insets",
      .label = "Edge insets",
      .value = std::format(
          "T {:.0f} · R {:.0f} · B {:.0f} · L {:.0f} mm",
          profile->edge_insets_mm.top, profile->edge_insets_mm.right,
          profile->edge_insets_mm.bottom, profile->edge_insets_mm.left),
  }));
  static constexpr std::array origin_labels{"Bottom Left", "Bottom Right",
                                            "Top Left", "Top Right"};
  static_cast<void>(ValueDisplay({
      .id = "origin-summary",
      .label = "Origin",
      .value = origin_labels[OriginIndex(profile->origin)],
  }));
}

void DrawMachinePresetBrowser(SettingsGalleryState &state) {
  MachineEditorState &editor = *state.machine_editor;
  ImGui::TextUnformatted("Choose a machine preset");
  detail::DrawSecondaryText(
      "Preset values remain editable and are not saved until Save machine.");
  if (Button({.id = "preset-back", .label = "Back to information"}).activated) {
    CloseMachinePresetPicker(state);
    return;
  }
  static constexpr std::array manufacturers{
      SelectOption{.id = "creality", .label = "Creality"},
      SelectOption{.id = "glowforge", .label = "Glowforge"},
      SelectOption{.id = "omtech", .label = "OMTech"},
  };
  std::size_t manufacturer_index = 0;
  for (std::size_t index = 0; index < manufacturers.size(); ++index) {
    if (manufacturers[index].label == editor.preset_manufacturer) {
      manufacturer_index = index;
    }
  }
  const SelectResult manufacturer = Select({
      .id = "preset-manufacturer",
      .label = "Manufacturer",
      .options = manufacturers,
      .selected_index = manufacturer_index,
  });
  if (manufacturer.changed) {
    static_cast<void>(SelectMachinePresetManufacturer(
        state, manufacturers[manufacturer.selected_index].label));
  }

  if (ImGui::BeginTable("##preset-cards", 2,
                        ImGuiTableFlags_SizingStretchSame)) {
    for (const MachinePreset &preset : kMachinePresets) {
      if (preset.manufacturer != editor.preset_manufacturer) {
        continue;
      }
      ImGui::TableNextColumn();
      ImGui::PushID(preset.id.data());
      if (ImGui::BeginChild("##preset", ImVec2(0.0f, Scale(132.0f)),
                            ImGuiChildFlags_Borders)) {
        ImGui::TextUnformatted(preset.model.data());
        detail::DrawSecondaryText(preset.variant);
        ImGui::Text("%.1f × %.1f mm", preset.bed_width_mm,
                    preset.bed_height_mm);
        if (Button({.id = "use-preset",
                    .label = "Use preset",
                    .variant = ButtonVariant::Primary})
                .activated) {
          static_cast<void>(ApplyMachinePreset(state, preset.id));
        }
      }
      ImGui::EndChild();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
}

void DrawMachineInformation(SettingsGalleryState &state) {
  if (!state.machine_editor.has_value()) {
    const MachineProfile *selected = SelectedMachine(state);
    ImGui::TextUnformatted("Machine Information");
    detail::DrawSecondaryText(
        "Open a local editor before changing a staged machine profile.");
    if (Button({.id = "information-edit",
                .label = selected == nullptr ? "Edit selected"
                                             : "Edit selected machine",
                .variant = ButtonVariant::Primary})
            .activated) {
      static_cast<void>(
          BeginEditMachine(state, state.draft.machines.selected_id));
    }
    ImGui::SameLine();
    if (Button({.id = "information-new", .label = "New machine"}).activated) {
      BeginNewMachine(state);
    }
    return;
  }
  if (state.machine_editor->preset_picker_open) {
    DrawMachinePresetBrowser(state);
    return;
  }

  MachineEditorState &editor = *state.machine_editor;
  MachineProfile &profile = editor.draft;
  ImGui::TextUnformatted(editor.mode == MachineEditorMode::New
                             ? "New machine profile"
                             : "Edit machine profile");
  detail::DrawSecondaryText(
      "Changes remain local to Machine Information until Save machine.");
  if (editor.mode == MachineEditorMode::New &&
      Button({.id = "choose-preset", .label = "Choose preset…"}).activated) {
    static_cast<void>(OpenMachinePresetPicker(state));
    return;
  }

  const TextInputResult name = TextInput({
      .id = "machine-name",
      .label = "Name",
      .value = profile.name,
      .validation = MachineEditorValidation(state, "name"),
  });
  if (name.changed) {
    profile.name = name.value;
    RefreshSettingsDerivedState(state);
  }
  if (Button(
          {.id = "make-default",
           .label = profile.is_default ? "Default machine" : "Make default",
           .availability = {.enabled = !profile.is_default,
                            .reason = "This profile is already the default"}})
          .activated) {
    profile.is_default = true;
    RefreshSettingsDerivedState(state);
  }

  ImGui::SeparatorText("Physical bed");
  DrawMachineEditorNumber(state, "bed-width", "Bed width", profile.bed_width_mm,
                          "bed_width_mm");
  DrawMachineEditorNumber(state, "bed-height", "Bed height",
                          profile.bed_height_mm, "bed_height_mm");
  ImGui::SeparatorText("Edge insets");
  DrawMachineEditorNumber(state, "inset-top", "Top", profile.edge_insets_mm.top,
                          "edge_insets_mm.top");
  DrawMachineEditorNumber(state, "inset-right", "Right",
                          profile.edge_insets_mm.right, "edge_insets_mm.right");
  DrawMachineEditorNumber(state, "inset-bottom", "Bottom",
                          profile.edge_insets_mm.bottom,
                          "edge_insets_mm.bottom");
  DrawMachineEditorNumber(state, "inset-left", "Left",
                          profile.edge_insets_mm.left, "edge_insets_mm.left");
  const UsableBedSize usable = UsableSize(profile);
  static_cast<void>(ValueDisplay({
      .id = "editor-usable-bed-size",
      .label = "Usable bed area",
      .value =
          std::format("{:.0f} × {:.0f} mm", usable.width_mm, usable.height_mm),
  }));
  if (const auto found = editor.errors.find("usable_width");
      found != editor.errors.end()) {
    ImGui::TextWrapped("%s", found->second.c_str());
  }
  if (const auto found = editor.errors.find("usable_height");
      found != editor.errors.end()) {
    ImGui::TextWrapped("%s", found->second.c_str());
  }

  static constexpr std::array origins{
      SelectOption{.id = "bottom-left", .label = "Bottom Left"},
      SelectOption{.id = "bottom-right", .label = "Bottom Right"},
      SelectOption{.id = "top-left", .label = "Top Left"},
      SelectOption{.id = "top-right", .label = "Top Right"},
  };
  const SelectResult origin = Select({
      .id = "origin",
      .label = "Origin location",
      .options = origins,
      .selected_index = OriginIndex(profile.origin),
  });
  if (origin.changed) {
    SetMachineEditorOrigin(state, OriginFromIndex(origin.selected_index));
  }
  if (editor.origin_confirmation_required && !editor.origin_confirmed) {
    StatusCard({
        .id = "origin-confirmation",
        .title = "Confirm the suggested origin",
        .message = "Verify the machine setup or choose another corner.",
        .status = SemanticStatus::Warning,
    });
    if (state.request_machine_confirmation_scroll) {
      ImGui::SetScrollHereY(0.85f);
      state.request_machine_confirmation_scroll = false;
    }
    if (Button({.id = "confirm-origin", .label = "Confirm suggested origin"})
            .activated) {
      ConfirmMachinePresetOrigin(state);
    }
  }

  const CheckboxResult thickness_enabled = Checkbox({
      .id = "thickness-enabled",
      .label = "Use default material thickness",
      .state = profile.material_thickness_mm.has_value() ? ToggleState::On
                                                         : ToggleState::Off,
  });
  if (thickness_enabled.changed) {
    if (thickness_enabled.state == ToggleState::On) {
      profile.material_thickness_mm = 18.0;
    } else {
      profile.material_thickness_mm.reset();
    }
    RefreshSettingsDerivedState(state);
  }
  if (profile.material_thickness_mm.has_value()) {
    DrawMachineEditorNumber(state, "material-thickness", "Material thickness",
                            *profile.material_thickness_mm,
                            "material_thickness_mm");
  }

  const bool valid = editor.errors.empty();
  if (Button({.id = "save-machine",
              .label = "Save machine",
              .variant = ButtonVariant::Primary,
              .availability = {.enabled = valid,
                               .reason =
                                   "Resolve Machine Information errors first"}})
          .activated) {
    static_cast<void>(SaveMachineEditor(state));
    return;
  }
  ImGui::SameLine();
  if (Button({.id = "cancel-machine-editor", .label = "Cancel"}).activated) {
    CancelMachineEditor(state);
  }
}

void DrawMachines(detail::UiAssetAtlas &assets, SettingsGalleryState &state) {
  DrawPageHeader(assets, "Machines",
                 "Define named machine beds once and reuse them throughout 3D "
                 "and Canvas.");
  if (!ImGui::BeginTabBar("##machine-tabs")) {
    return;
  }
  const auto tab = [&state](const char *label, const MachineSettingsTab id,
                            const auto &draw) {
    const ImGuiTabItemFlags flags = state.active_machine_tab == id
                                        ? ImGuiTabItemFlags_SetSelected
                                        : ImGuiTabItemFlags_None;
    const bool visible = ImGui::BeginTabItem(label, nullptr, flags);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
      state.active_machine_tab = id;
    }
    if (visible) {
      ImGui::Spacing();
      draw();
      ImGui::EndTabItem();
    }
  };
  tab("Profiles", MachineSettingsTab::Profiles,
      [&state] { DrawMachineProfiles(state); });
  tab("Usable Bed Area", MachineSettingsTab::BedArea,
      [&state] { DrawMachineBedArea(state); });
  tab("Machine Information", MachineSettingsTab::Information,
      [&state] { DrawMachineInformation(state); });
  ImGui::EndTabBar();
}

void DrawLicense(detail::UiAssetAtlas &assets, SettingsGalleryState &state) {
  DrawPageHeader(
      assets, "License",
      "Product activation is account-level state and applies immediately.");
  if (state.license.status == LicenseStatus::Active) {
    StatusCard({
        .id = "license-active",
        .title = "Export Face is activated",
        .message = "Licensed to SonderMill Studio · •••• •••• •••• 7K2Q",
        .status = SemanticStatus::Success,
        .icon = assets.Painter("success"),
    });
    if (Button({
                   .id = "deactivate-license",
                   .label = "Deactivate this computer",
                   .variant = ButtonVariant::Destructive,
               })
            .activated) {
      DeactivateLicense(state);
    }
    return;
  }

  StatusCard({
      .id = "license-inactive",
      .title = "Export Face is not activated",
      .message = "Enter a product key to enable licensed builds.",
      .status = state.license.status == LicenseStatus::Error
                    ? SemanticStatus::Failure
                    : SemanticStatus::Information,
      .icon = assets.Painter(state.license.status == LicenseStatus::Error
                                 ? "failure"
                                 : "information"),
  });
  ImGui::SeparatorText("Activate a product key");
  if (ImGui::BeginTable("##license-key-row", 2,
                        ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                            Scale(96.0f));
    ImGui::TableNextColumn();
    const TextInputResult key = TextInput({
        .id = "product-key",
        .label = "Product key",
        .value = state.license.key,
        .validation =
            {
                .invalid = !state.license.error.empty(),
                .message = state.license.error,
            },
    });
    if (key.changed) {
      state.license.key = key.value;
      state.license.error.clear();
      if (state.license.status == LicenseStatus::Error) {
        state.license.status = LicenseStatus::Inactive;
      }
    }
    ImGui::TableNextColumn();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                         ImGui::GetTextLineHeightWithSpacing());
    if (Button({.id = "activate-license",
                .label = "Activate",
                .variant = ButtonVariant::Primary,
                .size = {.x = -1.0f, .y = 32.0f}})
            .activated) {
      static_cast<void>(ActivateLicense(state));
    }
    ImGui::EndTable();
  }
  detail::DrawSecondaryTextWrapped(
      "Activation requires a valid product key. This gallery does not contact "
      "a license server.");
}

void DrawLegal(detail::UiAssetAtlas &assets) {
  DrawPageHeader(assets, "Legal",
                 "Application and third-party notices for Export Face 0.1.0.");
  ImGui::TextUnformatted("Export Face");
  ImGui::TextWrapped("Copyright © 2026 SonderMill. All rights reserved.");
  ImGui::SeparatorText("Third-party software");
  static constexpr std::array notices{
      std::pair{"Open CASCADE Technology", "GNU LGPL 2.1 with OCCT exception"},
      std::pair{"Dear ImGui", "MIT License"},
      std::pair{"Clipper2", "Boost Software License 1.0"},
      std::pair{"Boost", "Boost Software License 1.0"},
  };
  if (ImGui::BeginTable("##legal-notices", 3,
                        ImGuiTableFlags_Borders |
                            ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Component", ImGuiTableColumnFlags_WidthStretch,
                            1.3f);
    ImGui::TableSetupColumn("License", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                            Scale(112.0f));
    for (const auto &[component, license] : notices) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(component);
      ImGui::TableNextColumn();
      detail::DrawSecondaryText(license);
      ImGui::TableNextColumn();
      ImGui::PushID(component);
      if (Button({.id = "view-license",
                  .label = "View license",
                  .variant = ButtonVariant::Tertiary,
                  .size = {.x = -1.0f, .y = 28.0f}})
              .activated) {
        ImGui::OpenPopup("License text");
      }
      if (ImGui::BeginPopup("License text")) {
        ImGui::TextUnformatted(component);
        ImGui::Separator();
        ImGui::TextWrapped("%s", license);
        ImGui::EndPopup();
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::Spacing();
  ImGui::TextWrapped(
      "Complete license texts ship with the application distribution.");
}

void DrawSettingsPage(detail::UiAssetAtlas &assets, GalleryState &gallery) {
  switch (gallery.settings.active_section) {
  case SettingsSection::General:
    DrawGeneral(assets, gallery);
    break;
  case SettingsSection::Appearance:
    DrawAppearance(assets, gallery);
    break;
  case SettingsSection::Machines:
    DrawMachines(assets, gallery.settings);
    break;
  case SettingsSection::License:
    DrawLicense(assets, gallery.settings);
    break;
  case SettingsSection::Legal:
    DrawLegal(assets);
    break;
  }
}

void DrawSettingsNavigation(detail::UiAssetAtlas &assets,
                            SettingsGalleryState &state) {
  struct Section {
    SettingsSection id;
    const char *label;
    const char *icon;
  };
  static constexpr std::array sections{
      Section{SettingsSection::General, "General", "settings"},
      Section{SettingsSection::Appearance, "Appearance", "appearance"},
      Section{SettingsSection::Machines, "Machines", "machines"},
      Section{SettingsSection::License, "License", "license"},
      Section{SettingsSection::Legal, "Legal", "legal"},
  };
  for (const Section &section : sections) {
    const ButtonResult result = Button({
        .id = section.label,
        .label = section.label,
        .variant = ButtonVariant::Tertiary,
        .selected = state.active_section == section.id,
        .size = {.x = -1.0f, .y = 54.0f},
    });
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const float icon_size = Scale(16.0f);
    const float icon_left = minimum.x + Scale(10.0f);
    const float icon_top =
        minimum.y + (ImGui::GetItemRectSize().y - icon_size) * 0.5f;
    static_cast<void>(assets.DrawIcon(
        section.icon, steppenface::UiIconSize::Small16,
        {.minimum = {.x = icon_left, .y = icon_top},
         .maximum = {.x = icon_left + icon_size, .y = icon_top + icon_size}},
        state.active_section == section.id ? CurrentPalette().focus
                                           : CurrentPalette().text_secondary));
    if (result.activated) {
      state.active_section = section.id;
    }
  }
}

void RequestSettingsClose(GalleryState &gallery) {
  SettingsGalleryState &state = gallery.settings;
  if (state.dirty ||
      (state.machine_editor.has_value() && state.machine_editor->dirty)) {
    state.discard_confirmation_open = true;
    state.window_open = true;
    return;
  }
  state.window_open = false;
}

void DrawSettingsWindow(detail::UiAssetAtlas &assets, GalleryState &gallery) {
  SettingsGalleryState &state = gallery.settings;
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  const ImVec2 work_size = viewport->WorkSize;
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const ImVec2 maximum(
      std::max(Scale(320.0f), work_size.x - metrics.settings.inset * 2.0f),
      std::max(Scale(320.0f), work_size.y - metrics.settings.inset * 2.0f));
  const ImVec2 minimum(std::min(metrics.settings.minimum_width, maximum.x),
                       std::min(metrics.settings.minimum_height, maximum.y));
  if (state.request_window_focus) {
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + work_size.x * 0.5f,
                                   viewport->WorkPos.y + work_size.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(
        ImVec2(std::min(metrics.settings.width, maximum.x),
               std::min(metrics.settings.height, maximum.y)),
        ImGuiCond_Always);
    ImGui::SetNextWindowFocus();
    state.request_window_focus = false;
  }
  ImGui::SetNextWindowSizeConstraints(minimum, maximum);
  bool open = state.window_open;
  const SemanticPalette &palette = CurrentPalette();
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Scale(5.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(palette.surface.red, palette.surface.green,
                               palette.surface.blue, palette.surface.alpha));
  const bool visible = ImGui::Begin("System Settings##gallery", &open,
                                    ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse);
  const bool escape =
      ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
      ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (visible) {
    bool close_requested = false;
    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        ImVec4(palette.surface_raised.red, palette.surface_raised.green,
               palette.surface_raised.blue, palette.surface_raised.alpha));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(16.0f), Scale(6.0f)));
    if (ImGui::BeginChild("##settings-title-bar",
                          ImVec2(0.0f, metrics.settings.title_bar_height),
                          ImGuiChildFlags_AlwaysUseWindowPadding)) {
      detail::DrawSecondaryText("EXPORT FACE");
      if (assets.bold_font() != nullptr) {
        ImGui::PushFont(assets.bold_font());
      }
      ImGui::TextUnformatted("System Settings");
      if (assets.bold_font() != nullptr) {
        ImGui::PopFont();
      }
      ImGui::SameLine(ImGui::GetWindowWidth() - Scale(38.0f));
      close_requested = Button({.id = "settings-close",
                                .label = "×",
                                .variant = ButtonVariant::Tertiary,
                                .size = {.x = 28.0f, .y = 28.0f}})
                            .activated;
      ImGui::GetWindowDrawList()->AddLine(
          ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y +
                                              ImGui::GetWindowHeight() -
                                              metrics.geometry.border),
          ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(),
                 ImGui::GetWindowPos().y + ImGui::GetWindowHeight() -
                     metrics.geometry.border),
          ImGui::GetColorU32(ImGuiCol_Border), metrics.geometry.border);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    const float footer_height = Scale(52.0f);
    if (ImGui::BeginChild("##settings-content", ImVec2(0.0f, -footer_height),
                          false, ImGuiWindowFlags_NoScrollbar)) {
      if (ImGui::BeginTable("##settings-layout", 2,
                            ImGuiTableFlags_BordersInnerV |
                                ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Navigation", ImGuiTableColumnFlags_WidthFixed,
                                Scale(152.0f));
        ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            ImVec2(Scale(8.0f), Scale(8.0f)));
        if (ImGui::BeginChild("##settings-navigation", ImVec2(0.0f, 0.0f),
                              ImGuiChildFlags_AlwaysUseWindowPadding)) {
          DrawSettingsNavigation(assets, state);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::TableNextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            ImVec2(Scale(24.0f), Scale(16.0f)));
        if (ImGui::BeginChild("##settings-page", ImVec2(0.0f, 0.0f),
                              ImGuiChildFlags_AlwaysUseWindowPadding,
                              ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
          DrawSettingsPage(assets, gallery);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::EndTable();
      }
    }
    ImGui::EndChild();
    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        ImVec4(palette.surface_raised.red, palette.surface_raised.green,
               palette.surface_raised.blue, palette.surface_raised.alpha));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(16.0f), Scale(10.0f)));
    ImGui::BeginChild("##settings-footer", ImVec2(0.0f, footer_height),
                      ImGuiChildFlags_Borders |
                          ImGuiChildFlags_AlwaysUseWindowPadding);
    const std::string status =
        state.machine_editor.has_value()
            ? "Save or cancel Machine Information before applying"
        : !state.errors.empty()
            ? std::to_string(state.errors.size()) +
                  (state.errors.size() == 1 ? " field needs attention"
                                            : " fields need attention")
        : state.dirty ? "Unapplied changes"
                      : "Settings are up to date";
    ImGui::TextUnformatted(status.c_str());
    const float button_width = Scale(176.0f);
    ImGui::SameLine(std::max(ImGui::GetCursorPosX(),
                             ImGui::GetWindowWidth() - button_width));
    if (Button({
                   .id = "settings-cancel",
                   .label = "Cancel",
                   .size = {.x = 80.0f, .y = 32.0f},
               })
            .activated) {
      DiscardSettings(state);
      gallery.theme = ResolveSettingsTheme(state.applied.appearance.theme,
                                           state.system_theme);
      state.window_open = false;
    }
    ImGui::SameLine();
    if (Button(
            {
                .id = "settings-apply",
                .label = "Apply",
                .variant = ButtonVariant::Primary,
                .availability =
                    {
                        .enabled = !state.machine_editor.has_value() &&
                                   state.dirty && state.errors.empty(),
                        .reason = state.machine_editor.has_value()
                                      ? "Save or cancel Machine Information "
                                        "before Apply"
                                  : !state.errors.empty()
                                      ? "Resolve validation errors before Apply"
                                      : "No unapplied settings changes",
                    },
                .size = {.x = 80.0f, .y = 32.0f},
            })
            .activated) {
      static_cast<void>(ApplySettings(state));
      gallery.theme = ResolveSettingsTheme(state.applied.appearance.theme,
                                           state.system_theme);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    if (close_requested) {
      RequestSettingsClose(gallery);
    }
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
  if (!open || escape) {
    RequestSettingsClose(gallery);
  }
}

void DrawDiscardConfirmation(GalleryState &gallery) {
  SettingsGalleryState &state = gallery.settings;
  if (state.discard_confirmation_open) {
    ImGui::OpenPopup("Discard System Settings changes?");
    state.discard_confirmation_open = false;
  }
  if (ImGui::BeginPopupModal("Discard System Settings changes?", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped(
        "Discard unapplied General, Appearance, and Machines changes?");
    if (Button({
                   .id = "discard-settings",
                   .label = "Discard changes",
                   .variant = ButtonVariant::Destructive,
               })
            .activated) {
      DiscardSettings(state);
      gallery.theme = ResolveSettingsTheme(state.applied.appearance.theme,
                                           state.system_theme);
      state.window_open = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (Button({
                   .id = "keep-editing-settings",
                   .label = "Keep editing",
               })
            .activated) {
      state.window_open = true;
      state.request_window_focus = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

} // namespace

void DrawSettingsGallery(detail::UiAssetAtlas &assets, GalleryState &state) {
  detail::DrawSecondaryText(
      "One modeless System Settings window with staged preferences, immediate "
      "license actions, and read-only legal notices.");
  ImGui::Spacing();
  if (!state.settings.window_open) {
    if (Button({
                   .id = "open-system-settings",
                   .label = "Open System Settings",
                   .variant = ButtonVariant::Primary,
               })
            .activated) {
      state.settings.window_open = true;
      state.settings.request_window_focus = true;
    }
  } else {
    ImGui::TextWrapped(
        "The window remains modeless. Use the five section buttons to inspect "
        "the complete contract.");
  }
}

void DrawSettingsGalleryWindow(detail::UiAssetAtlas &assets,
                               GalleryState &state) {
  if (state.settings.window_open) {
    DrawSettingsWindow(assets, state);
  }
  DrawDiscardConfirmation(state);
}

} // namespace fancy_ui::gallery
