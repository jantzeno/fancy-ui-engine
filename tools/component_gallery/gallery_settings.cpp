#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
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

  const SectionResult files = BeginSection({
      .id = "settings-general-files",
      .heading = "Files",
  });
  if (files.visible) {
    const TextInputResult open = TextInput({
        .id = "default-open-directory",
        .label = "Open/import directory",
        .value = general.default_open_directory,
        .validation = ValidationFor(state, "general.default_open_directory"),
    });
    if (open.changed) {
      general.default_open_directory = open.value;
      RefreshSettingsDerivedState(state);
    }
    if (Button({
                   .id = "browse-open",
                   .label = "Browse…",
                   .variant = ButtonVariant::Secondary,
               })
            .activated) {
      general.default_open_directory = "~/Documents/CNC";
      RefreshSettingsDerivedState(state);
    }

    const TextInputResult export_path = TextInput({
        .id = "default-export-directory",
        .label = "Export directory",
        .value = general.default_export_directory,
        .validation = ValidationFor(state, "general.default_export_directory"),
    });
    if (export_path.changed) {
      general.default_export_directory = export_path.value;
      RefreshSettingsDerivedState(state);
    }
    if (Button({
                   .id = "browse-export",
                   .label = "Browse…",
                   .variant = ButtonVariant::Secondary,
               })
            .activated) {
      general.default_export_directory = "~/Exports/CNC";
      RefreshSettingsDerivedState(state);
    }
  }
  EndSection(files);

  const SectionResult startup = BeginSection({
      .id = "settings-general-startup",
      .heading = "Startup",
  });
  if (startup.visible) {
    ImGui::TextUnformatted("Default view");
    if (Button({
                   .id = "default-view-model",
                   .label = "3D",
                   .selected = !general.default_canvas_view,
                   .size = {.x = 72.0f, .y = 32.0f},
               })
            .activated) {
      general.default_canvas_view = false;
      RefreshSettingsDerivedState(state);
    }
    ImGui::SameLine();
    if (Button({
                   .id = "default-view-canvas",
                   .label = "Canvas",
                   .selected = general.default_canvas_view,
                   .size = {.x = 88.0f, .y = 32.0f},
               })
            .activated) {
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
  EndSection(startup);
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
    ImGui::TextDisabled("1 object selected · Ready to edit");
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
  };
  static constexpr std::array options{
      ThemeOption{SettingsThemeChoice::System, "System"},
      ThemeOption{SettingsThemeChoice::Light, "Light"},
      ThemeOption{SettingsThemeChoice::Dark, "Dark"},
  };
  for (std::size_t index = 0; index < options.size(); ++index) {
    const ThemeOption &option = options[index];
    if (index > 0) {
      ImGui::SameLine();
    }
    if (Button({
                   .id = option.label,
                   .label = option.label,
                   .selected = state.draft.appearance.theme == option.choice,
                   .size = {.x = 96.0f, .y = 32.0f},
               })
            .activated) {
      state.draft.appearance.theme = option.choice;
      RefreshSettingsDerivedState(state);
      gallery.theme = ResolveSettingsTheme(option.choice, state.system_theme);
    }
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

bool DrawMachineNumber(SettingsGalleryState &state, const char *id,
                       const char *label, double &value,
                       const std::string &error_path) {
  const NumericInputResult result = NumericInput({
      .id = id,
      .label = label,
      .unit = "mm",
      .value = value,
      .format = "%.1f",
      .validation = ValidationFor(state, error_path),
  });
  if (result.changed) {
    value = result.value;
    RefreshSettingsDerivedState(state);
  }
  return result.changed;
}

void DrawMachines(detail::UiAssetAtlas &assets, SettingsGalleryState &state) {
  DrawPageHeader(assets, "Machines",
                 "Define named machine beds once and reuse them throughout 3D "
                 "and Canvas.");
  const float list_width =
      std::min(Scale(224.0f), ImGui::GetContentRegionAvail().x * 0.34f);
  if (ImGui::BeginTable("##machine-editor", 2,
                        ImGuiTableFlags_BordersInnerV |
                            ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Profiles", ImGuiTableColumnFlags_WidthFixed,
                            list_width);
    ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Profiles");
    ImGui::SameLine();
    if (Button({
                   .id = "add-machine",
                   .label = "Add",
                   .variant = ButtonVariant::Tertiary,
                   .size = {.x = 56.0f, .y = 28.0f},
               })
            .activated) {
      AddMachine(state);
    }
    for (const MachineProfile &profile : state.draft.machines.profiles) {
      const std::string label =
          profile.name + (profile.is_default ? " · Default" : "");
      if (Button({
                     .id = profile.id,
                     .label = label,
                     .variant = ButtonVariant::Tertiary,
                     .selected = profile.id == state.draft.machines.selected_id,
                     .size = {.x = -1.0f, .y = 32.0f},
                 })
              .activated) {
        state.draft.machines.selected_id = profile.id;
        RefreshSettingsDerivedState(state);
      }
    }
    if (Button({
                   .id = "duplicate-machine",
                   .label = "Duplicate",
                   .size = {.x = 96.0f, .y = 32.0f},
               })
            .activated) {
      DuplicateSelectedMachine(state);
    }
    ImGui::SameLine();
    if (Button({
                   .id = "remove-machine",
                   .label = "Remove",
                   .variant = ButtonVariant::Destructive,
                   .availability =
                       {
                           .enabled = CanRemoveSelectedMachine(state),
                           .reason =
                               "The default or final machine cannot be removed",
                       },
                   .size = {.x = 88.0f, .y = 32.0f},
               })
            .activated) {
      ImGui::OpenPopup("Remove machine profile");
    }
    if (ImGui::BeginPopupModal("Remove machine profile", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      const MachineProfile *selected = SelectedMachine(state);
      ImGui::TextWrapped("Remove %s from the staged machine profiles?",
                         selected == nullptr ? "this machine"
                                             : selected->name.c_str());
      if (Button({
                     .id = "confirm-remove",
                     .label = "Remove",
                     .variant = ButtonVariant::Destructive,
                 })
              .activated) {
        static_cast<void>(RemoveSelectedMachine(state));
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (Button({
                     .id = "cancel-remove",
                     .label = "Keep machine",
                 })
              .activated) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::TableNextColumn();
    MachineProfile *profile = SelectedMachine(state);
    if (profile != nullptr) {
      ImGui::PushID(profile->id.c_str());
      const std::string name_path = "machines." + profile->id + ".name";
      const TextInputResult name = TextInput({
          .id = "machine-name",
          .label = "Name",
          .value = profile->name,
          .validation = ValidationFor(state, name_path),
      });
      if (name.changed) {
        profile->name = name.value;
        RefreshSettingsDerivedState(state);
      }
      if (Button({
                     .id = "make-default",
                     .label = profile->is_default ? "Default machine"
                                                  : "Make default",
                     .availability =
                         {
                             .enabled = !profile->is_default,
                             .reason = "This profile is already the default",
                         },
                 })
              .activated) {
        SetSelectedMachineDefault(state);
        profile = SelectedMachine(state);
      }

      const std::string prefix = "machines." + profile->id + ".";
      DrawMachineNumber(state, "bed-width", "Bed width", profile->bed_width_mm,
                        prefix + "bed_width_mm");
      DrawMachineNumber(state, "bed-height", "Bed height",
                        profile->bed_height_mm, prefix + "bed_height_mm");
      ImGui::SeparatorText("Edge insets");
      DrawMachineNumber(state, "inset-top", "Top", profile->edge_insets_mm.top,
                        prefix + "edge_insets_mm.top");
      DrawMachineNumber(state, "inset-right", "Right",
                        profile->edge_insets_mm.right,
                        prefix + "edge_insets_mm.right");
      DrawMachineNumber(state, "inset-bottom", "Bottom",
                        profile->edge_insets_mm.bottom,
                        prefix + "edge_insets_mm.bottom");
      DrawMachineNumber(state, "inset-left", "Left",
                        profile->edge_insets_mm.left,
                        prefix + "edge_insets_mm.left");
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
          .selected_index = OriginIndex(profile->origin),
      });
      if (origin.changed) {
        profile->origin = OriginFromIndex(origin.selected_index);
        RefreshSettingsDerivedState(state);
      }

      const CheckboxResult thickness_enabled = Checkbox({
          .id = "thickness-enabled",
          .label = "Use default material thickness",
          .state = profile->material_thickness_mm.has_value()
                       ? ToggleState::On
                       : ToggleState::Off,
      });
      if (thickness_enabled.changed) {
        if (thickness_enabled.state == ToggleState::On) {
          profile->material_thickness_mm = 18.0;
        } else {
          profile->material_thickness_mm.reset();
        }
        RefreshSettingsDerivedState(state);
      }
      if (profile->material_thickness_mm.has_value()) {
        DrawMachineNumber(state, "material-thickness", "Material thickness",
                          *profile->material_thickness_mm,
                          prefix + "material_thickness_mm");
      }

      const UsableBedSize usable = UsableSize(*profile);
      static_cast<void>(ValueDisplay({
          .id = "usable-bed-size",
          .label = "Usable bed size",
          .value = std::format("{:.0f} × {:.0f} mm", usable.width_mm,
                               usable.height_mm),
      }));
      if (const auto error = state.errors.find(prefix + "usable_width");
          error != state.errors.end()) {
        ImGui::TextWrapped("%s", error->second.c_str());
      }
      if (const auto error = state.errors.find(prefix + "usable_height");
          error != state.errors.end()) {
        ImGui::TextWrapped("%s", error->second.c_str());
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
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
  if (Button({
                 .id = "activate-license",
                 .label = "Activate",
                 .variant = ButtonVariant::Primary,
             })
          .activated) {
    static_cast<void>(ActivateLicense(state));
  }
  ImGui::TextWrapped(
      "This gallery validates the key format and never contacts a server.");
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
  for (const auto &[component, license] : notices) {
    static_cast<void>(ValueDisplay({
        .id = component,
        .label = component,
        .value = license,
        .label_width = 208.0f,
    }));
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
  if (state.dirty) {
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
  const ImVec2 maximum(std::max(Scale(320.0f), work_size.x - Scale(32.0f)),
                       std::max(Scale(320.0f), work_size.y - Scale(32.0f)));
  const ImVec2 minimum(std::min(Scale(720.0f), maximum.x),
                       std::min(Scale(480.0f), maximum.y));
  if (state.request_window_focus) {
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + work_size.x * 0.5f,
                                   viewport->WorkPos.y + work_size.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(Scale(880.0f), maximum.x),
                                    std::min(Scale(560.0f), maximum.y)),
                             ImGuiCond_Always);
    ImGui::SetNextWindowFocus();
    state.request_window_focus = false;
  }
  ImGui::SetNextWindowSizeConstraints(minimum, maximum);
  bool open = state.window_open;
  const SemanticPalette &palette = CurrentPalette();
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Scale(5.0f));
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(palette.surface.red, palette.surface.green,
                               palette.surface.blue, palette.surface.alpha));
  const bool visible = ImGui::Begin("System Settings##gallery", &open,
                                    ImGuiWindowFlags_NoSavedSettings);
  const bool escape =
      ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
      ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (visible) {
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
        DrawSettingsNavigation(assets, state);
        ImGui::TableNextColumn();
        if (ImGui::BeginChild("##settings-page", ImVec2(0.0f, 0.0f), false,
                              ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
          DrawSettingsPage(assets, gallery);
        }
        ImGui::EndChild();
        ImGui::EndTable();
      }
    }
    ImGui::EndChild();
    ImGui::Separator();
    const std::string status =
        !state.errors.empty()
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
                        .enabled = state.dirty && state.errors.empty(),
                        .reason = !state.errors.empty()
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
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
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
  ImGui::TextDisabled(
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
