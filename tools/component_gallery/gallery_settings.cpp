#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <format>
#include <string>
#include <string_view>
#include <tuple>

namespace fancy_ui::gallery {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

ImU32 Packed(const ColorRgba color) {
  return ImGui::GetColorU32(ToImVec4(color));
}

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
                    const char *description,
                    const float bottom_padding = 20.0f) {
  if (assets.heading_font() != nullptr) {
    ImGui::PushFont(
        assets.heading_font(),
        CurrentLayoutMetrics().typography.settings_title_font_height);
  }
  ImGui::TextUnformatted(title);
  if (assets.heading_font() != nullptr) {
    ImGui::PopFont();
  }
  ImGui::TextWrapped("%s", description);
  if (bottom_padding > 0.0f) {
    ImGui::Dummy(ImVec2(0.0f, Scale(bottom_padding)));
  }
}

void DrawRule() {
  const ImVec2 start = ImGui::GetCursorScreenPos();
  ImGui::GetWindowDrawList()->AddLine(
      start, ImVec2(start.x + ImGui::GetContentRegionAvail().x, start.y),
      ImGui::GetColorU32(ImGuiCol_Border),
      CurrentLayoutMetrics().geometry.border);
  ImGui::Dummy(ImVec2(0.0f, Scale(2.0f)));
}

void DrawDashedLine(ImDrawList &draw_list, const ImVec2 start, const ImVec2 end,
                    const ImU32 color) {
  const ImVec2 delta(end.x - start.x, end.y - start.y);
  const float length = std::hypot(delta.x, delta.y);
  if (length <= 0.0f) {
    return;
  }
  const ImVec2 direction(delta.x / length, delta.y / length);
  for (float offset = 0.0f; offset < length; offset += Scale(10.0f)) {
    const float dash_end = std::min(offset + Scale(6.0f), length);
    draw_list.AddLine(
        ImVec2(start.x + direction.x * offset, start.y + direction.y * offset),
        ImVec2(start.x + direction.x * dash_end,
               start.y + direction.y * dash_end),
        color, CurrentLayoutMetrics().geometry.focus_ring);
  }
}

void DrawDashedRect(ImDrawList &draw_list, const ImVec2 minimum,
                    const ImVec2 maximum, const ImU32 color) {
  DrawDashedLine(draw_list, minimum, ImVec2(maximum.x, minimum.y), color);
  DrawDashedLine(draw_list, ImVec2(maximum.x, minimum.y), maximum, color);
  DrawDashedLine(draw_list, maximum, ImVec2(minimum.x, maximum.y), color);
  DrawDashedLine(draw_list, ImVec2(minimum.x, maximum.y), minimum, color);
}

void DrawStrongText(ImFont *font, const std::string_view text) {
  if (font != nullptr) {
    ImGui::PushFont(font, CurrentLayoutMetrics().typography.body_font_height);
  }
  ImGui::TextUnformatted(text.data(), text.data() + text.size());
  if (font != nullptr) {
    ImGui::PopFont();
  }
}

void DrawSectionHeading(const char *title, const bool ruled = false,
                        ImFont *font = nullptr) {
  if (ruled) {
    DrawRule();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(6.0f));
  }
  DrawStrongText(font, title);
}

void DrawStackedLabel(const char *label) {
  detail::DrawSecondaryText(label);
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - Scale(2.0f));
}

TextInputResult DrawStackedTextInput(const TextInputSpec &spec) {
  DrawStackedLabel(spec.label.data());
  TextInputSpec control = spec;
  control.label = {};
  return TextInput(control);
}

void DrawSecondaryAt(const ImVec2 position, const char *text,
                     const ColorRgba color) {
  ImGui::GetWindowDrawList()->AddText(position, Packed(color), text);
}

void DrawGeneral(detail::UiAssetAtlas &assets, GalleryState &gallery) {
  SettingsGalleryState &state = gallery.settings;
  GeneralSettings &general = state.draft.general;
  DrawPageHeader(
      assets, "General",
      "Choose file locations, the startup workspace, and optional diagnostic "
      "tools.");

  DrawSectionHeading("Files", false, assets.heading_font());
  const auto draw_directory =
      [&state](const char *id, const char *label, std::string &value,
               const char *validation_path, const char *browse_id,
               const char *browse_value) {
        DrawStackedLabel(label);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp)) {
          ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
          ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                                  Scale(91.0f));
          ImGui::TableNextColumn();
          const TextInputResult input = TextInput({
              .id = id,
              .label = {},
              .value = value,
              .validation = ValidationFor(state, validation_path),
          });
          if (input.changed) {
            value = input.value;
            RefreshSettingsDerivedState(state);
          }
          ImGui::TableNextColumn();
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Scale(8.0f));
          if (Button({.id = browse_id,
                      .label = "Browse…",
                      .variant = ButtonVariant::Secondary,
                      .size = {.x = -1.0f, .y = 32.0f}})
                  .activated) {
            value = browse_value;
            RefreshSettingsDerivedState(state);
          }
          ImGui::EndTable();
        }
        ImGui::PopStyleVar();
      };
  draw_directory("default-open-directory", "Default open/import directory",
                 general.default_open_directory,
                 "general.default_open_directory", "browse-open",
                 "~/Documents/CNC");
  draw_directory("default-export-directory", "Default export directory",
                 general.default_export_directory,
                 "general.default_export_directory", "browse-export",
                 "~/Exports/CNC");

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(16.0f));
  DrawSectionHeading("Startup", true, assets.heading_font());
  DrawStackedLabel("Default view");
  static constexpr std::array views{
      SelectOption{.id = "model", .label = "3D"},
      SelectOption{.id = "canvas", .label = "Canvas"},
  };
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(ImGui::GetStyle().FramePadding.x, 0.0f));
  const RadioGroupResult view = RadioGroup({
      .id = "default-view",
      .options = views,
      .selected_index = general.default_canvas_view ? 1U : 0U,
      .layout = RadioGroupLayout::Horizontal,
  });
  ImGui::PopStyleVar();
  if (view.changed) {
    general.default_canvas_view = view.selected_index == 1;
    RefreshSettingsDerivedState(state);
  }
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(3.0f));
  const float description_left = ImGui::GetCursorPosX() + Scale(29.0f);
  if (assets.heading_font() != nullptr) {
    ImGui::PushFont(assets.heading_font(),
                    CurrentLayoutMetrics().typography.body_font_height);
  }
  const CheckboxResult diagnostics = Checkbox({
      .id = "diagnostics-enabled",
      .label = "Enable diagnostics",
      .state = general.diagnostics_enabled ? ToggleState::On : ToggleState::Off,
  });
  if (assets.heading_font() != nullptr) {
    ImGui::PopFont();
  }
  if (diagnostics.changed) {
    general.diagnostics_enabled = diagnostics.state == ToggleState::On;
    RefreshSettingsDerivedState(state);
  }
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - Scale(11.0f));
  ImGui::SetCursorPosX(description_left);
  detail::DrawSecondaryTextWrapped(
      "Adds Diagnostics to the Canvas activity bar. Intended for fixture and "
      "troubleshooting tools.");
}

void DrawThemePreview(const ResolvedTheme preview, ImFont *heading_font) {
  const SemanticPalette palette = PaletteFor(preview);
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(palette.surface_muted));
  ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(palette.border_strong));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginChild("##settings-theme-preview", ImVec2(0.0f, Scale(128.0f)),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 minimum = ImGui::GetWindowPos();
    const ImVec2 maximum(minimum.x + ImGui::GetWindowWidth(),
                         minimum.y + ImGui::GetWindowHeight());
    const float bar_height = Scale(36.0f);
    const float rail_width = Scale(34.0f);
    draw_list->AddRectFilled(minimum, ImVec2(maximum.x, minimum.y + bar_height),
                             Packed(palette.application_surface));
    draw_list->AddLine(ImVec2(minimum.x, minimum.y + bar_height),
                       ImVec2(maximum.x, minimum.y + bar_height),
                       Packed(palette.border), metrics.geometry.border);
    draw_list->AddRectFilled(ImVec2(minimum.x, minimum.y + bar_height),
                             ImVec2(minimum.x + rail_width, maximum.y),
                             Packed(palette.application_surface));
    draw_list->AddRectFilled(ImVec2(minimum.x, minimum.y + bar_height),
                             ImVec2(minimum.x + Scale(3.0f), maximum.y),
                             Packed(palette.focus));
    draw_list->AddLine(ImVec2(minimum.x + rail_width, minimum.y + bar_height),
                       ImVec2(minimum.x + rail_width, maximum.y),
                       Packed(palette.border), metrics.geometry.border);
    const auto draw_strong = [draw_list, heading_font, &palette](
                                 const ImVec2 position, const char *text) {
      if (heading_font != nullptr) {
        draw_list->AddText(heading_font,
                           CurrentLayoutMetrics().typography.body_font_height,
                           position, Packed(palette.text_primary), text);
      } else {
        draw_list->AddText(position, Packed(palette.text_primary), text);
      }
    };
    draw_strong(ImVec2(minimum.x + Scale(9.0f), minimum.y + Scale(10.0f)),
                "Export Face");
    const auto draw_command = [&](const float right, const char *label,
                                  const bool primary) {
      const ImVec2 text_size = ImGui::CalcTextSize(label);
      const float width = text_size.x + Scale(20.0f);
      const ImVec2 button_min(minimum.x + right - width,
                              minimum.y + Scale(6.0f));
      const ImVec2 button_max(minimum.x + right, minimum.y + Scale(30.0f));
      draw_list->AddRectFilled(
          button_min, button_max,
          Packed(primary ? palette.action_primary : palette.surface_raised),
          metrics.geometry.control_radius);
      draw_list->AddRect(
          button_min, button_max,
          Packed(primary ? palette.action_primary : palette.border_strong),
          metrics.geometry.control_radius, 0, metrics.geometry.border);
      draw_list->AddText(
          ImVec2(
              std::floor((button_min.x + button_max.x - text_size.x) * 0.5f),
              std::floor((button_min.y + button_max.y - text_size.y) * 0.5f)),
          Packed(primary ? palette.on_emphasis : palette.text_primary), label);
    };
    draw_command(ImGui::GetWindowWidth() - Scale(143.0f), "Open", false);
    draw_command(ImGui::GetWindowWidth() - Scale(8.0f), "Export", true);
    const float content_x = minimum.x + rail_width + Scale(13.0f);
    draw_strong(ImVec2(content_x, minimum.y + bar_height + Scale(13.0f)),
                "Frame plate");
    draw_list->AddText(ImVec2(content_x, minimum.y + bar_height + Scale(35.0f)),
                       Packed(palette.text_secondary),
                       "1 object selected · Ready to edit");
    draw_list->AddText(ImVec2(content_x, minimum.y + bar_height + Scale(57.0f)),
                       Packed(palette.success), "No geometry issues");
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);
}

void DrawAppearance(detail::UiAssetAtlas &assets, GalleryState &gallery) {
  SettingsGalleryState &state = gallery.settings;
  DrawPageHeader(
      assets, "Appearance",
      "Theme changes preview immediately and remain reversible until Apply.");
  DrawSectionHeading("Theme", false, assets.heading_font());
  struct ThemeOption {
    SettingsThemeChoice choice;
    const char *label;
    const char *description_first;
    const char *description_second;
  };
  static constexpr std::array options{
      ThemeOption{SettingsThemeChoice::System, "System",
                  "Follow the operating-", "system appearance"},
      ThemeOption{SettingsThemeChoice::Light, "Light", "Use the light semantic",
                  "palette"},
      ThemeOption{SettingsThemeChoice::Dark, "Dark", "Use the dark semantic",
                  "palette"},
  };
  const SemanticPalette &palette = CurrentPalette();
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(Scale(4.0f), 0.0f));
  if (ImGui::BeginTable("##theme-options", 3,
                        ImGuiTableFlags_SizingStretchSame)) {
    for (const ThemeOption &option : options) {
      ImGui::TableNextColumn();
      ImGui::PushID(option.label);
      const bool selected = state.draft.appearance.theme == option.choice;
      const bool activated =
          ImGui::InvisibleButton("##theme-card", ImVec2(-1.0f, Scale(76.0f)),
                                 ImGuiButtonFlags_EnableNav);
      const InteractionResult interaction = detail::CaptureInteraction();
      const ImVec2 minimum = ImGui::GetItemRectMin();
      const ImVec2 maximum = ImGui::GetItemRectMax();
      const ColorRgba fill = selected              ? palette.selection
                             : interaction.hovered ? palette.control_hover
                                                   : palette.surface_raised;
      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      draw_list->AddRectFilled(minimum, maximum, Packed(fill),
                               metrics.geometry.surface_radius);
      draw_list->AddRect(
          minimum, maximum,
          Packed(selected ? palette.focus : palette.border_strong),
          metrics.geometry.surface_radius, 0,
          selected ? metrics.geometry.focus_ring : metrics.geometry.border);
      if (assets.heading_font() != nullptr) {
        draw_list->AddText(
            assets.heading_font(), metrics.typography.body_font_height,
            ImVec2(minimum.x + Scale(12.0f), minimum.y + Scale(10.0f)),
            Packed(selected ? palette.focus : palette.text_primary),
            option.label);
      } else {
        draw_list->AddText(
            ImVec2(minimum.x + Scale(12.0f), minimum.y + Scale(10.0f)),
            Packed(selected ? palette.focus : palette.text_primary),
            option.label);
      }
      DrawSecondaryAt(
          ImVec2(minimum.x + Scale(12.0f), minimum.y + Scale(33.0f)),
          option.description_first, palette.text_secondary);
      DrawSecondaryAt(
          ImVec2(minimum.x + Scale(12.0f), minimum.y + Scale(50.0f)),
          option.description_second, palette.text_secondary);
      detail::DrawFocusRing(interaction);
      if (activated) {
        state.draft.appearance.theme = option.choice;
        RefreshSettingsDerivedState(state);
        gallery.theme = ResolveSettingsTheme(option.choice, state.system_theme);
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(14.0f));
  DrawSectionHeading("Preview", true, assets.heading_font());
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(4.0f));
  DrawThemePreview(
      ResolveSettingsTheme(state.draft.appearance.theme, state.system_theme),
      assets.heading_font());
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

const char *OriginLabel(const MachineOrigin origin) {
  static constexpr std::array labels{"Bottom Left", "Bottom Right", "Top Left",
                                     "Top Right"};
  return labels[OriginIndex(origin)];
}

void DrawBadge(const std::string_view text,
               const ColorRgba color = CurrentPalette().focus) {
  const std::string label(text);
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  const ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
  const ImVec2 maximum(minimum.x + text_size.x + Scale(10.0f),
                       minimum.y + text_size.y + Scale(4.0f));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRect(minimum, maximum, Packed(color),
                     CurrentLayoutMetrics().geometry.control_radius, 0,
                     CurrentLayoutMetrics().geometry.border);
  draw_list->AddText(ImVec2(minimum.x + Scale(5.0f), minimum.y + Scale(2.0f)),
                     Packed(color), label.c_str());
  ImGui::Dummy(ImVec2(maximum.x - minimum.x, maximum.y - minimum.y));
}

void DrawKicker(const std::string_view text) {
  detail::DrawSecondaryText(text);
}

Validation MachineEditorValidation(const SettingsGalleryState &state,
                                   const std::string &path) {
  if (!state.machine_editor.has_value()) {
    return {};
  }
  if (state.machine_editor->mode == MachineEditorMode::New &&
      !state.machine_editor->dirty) {
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
  const Validation validation = MachineEditorValidation(state, error_path);
  const bool required_dimension =
      error_path == "bed_width_mm" || error_path == "bed_height_mm";
  DrawStackedLabel(label);
  ImGui::PushID(id);
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
  const bool table = ImGui::BeginTable("##number-row", 2,
                                       ImGuiTableFlags_SizingStretchProp |
                                           ImGuiTableFlags_NoSavedSettings);
  if (table) {
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Unit", ImGuiTableColumnFlags_WidthFixed,
                            Scale(34.0f));
    ImGui::TableNextColumn();
  }
  bool changed = false;
  if (value == 0.0 && required_dimension) {
    const TextInputResult result = TextInput({
        .id = "value",
        .label = {},
        .value = {},
        .placeholder = "Required",
        .validation = validation,
    });
    changed = result.changed;
    if (result.changed && !result.value.empty()) {
      double parsed = 0.0;
      const auto [end, error] =
          std::from_chars(result.value.data(),
                          result.value.data() + result.value.size(), parsed);
      if (error == std::errc{} &&
          end == result.value.data() + result.value.size()) {
        value = parsed;
      }
    }
  } else {
    const NumericInputResult result = NumericInput({
        .id = "value",
        .label = {},
        .value = value,
        .format = "%.0f",
        .validation = validation,
    });
    changed = result.changed;
    if (result.changed) {
      value = result.value;
    }
  }
  if (table) {
    ImGui::TableNextColumn();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Scale(8.0f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(7.0f));
    detail::DrawSecondaryText("mm");
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  ImGui::PopID();
  if (changed) {
    RefreshSettingsDerivedState(state);
  }
  return changed;
}

void DrawMachineProfiles(SettingsGalleryState &state, ImFont *heading_font) {
  const std::size_t profile_count = state.draft.machines.profiles.size();
  const float heading_y = ImGui::GetCursorPosY();
  DrawStrongText(heading_font, "Machine profiles");
  detail::DrawSecondaryText(
      "Select the profile used by the bed summary and profile actions.");
  const std::string count = std::to_string(profile_count) +
                            (profile_count == 1 ? " profile" : " profiles");
  const float badge_width = ImGui::CalcTextSize(count.c_str()).x + Scale(10.0f);
  ImGui::SetCursorPos(
      ImVec2(ImGui::GetWindowWidth() - badge_width - Scale(12.0f), heading_y));
  DrawBadge(count);
  ImGui::SetCursorPosY(heading_y + Scale(48.0f));

  const float rows_height = Scale(62.0f * static_cast<float>(profile_count));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginChild("##machine-profile-rows", ImVec2(0.0f, rows_height),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,
                        ImVec2(Scale(8.0f), Scale(6.0f)));
    if (ImGui::BeginTable("##profiles", 4,
                          ImGuiTableFlags_SizingStretchProp |
                              ImGuiTableFlags_NoSavedSettings)) {
      ImGui::TableSetupColumn("Identity", ImGuiTableColumnFlags_WidthStretch,
                              1.4f);
      ImGui::TableSetupColumn("Physical", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Usable", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                              Scale(88.0f));
      for (const MachineProfile &profile : state.draft.machines.profiles) {
        const bool selected = profile.id == state.draft.machines.selected_id;
        ImGui::PushID(profile.id.c_str());
        ImGui::TableNextRow(ImGuiTableRowFlags_None, Scale(62.0f));
        const ImVec2 row_min(ImGui::GetWindowPos().x,
                             ImGui::GetCursorScreenPos().y - Scale(6.0f));
        const ImVec2 row_max(row_min.x + ImGui::GetWindowWidth(),
                             row_min.y + Scale(62.0f));
        if (selected) {
          ImGui::GetWindowDrawList()->AddRectFilled(
              row_min, row_max, Packed(CurrentPalette().selection));
          ImGui::GetWindowDrawList()->AddRect(
              row_min, row_max, Packed(CurrentPalette().focus), 0.0f, 0,
              CurrentLayoutMetrics().geometry.border);
        } else if (&profile != &state.draft.machines.profiles.front()) {
          ImGui::GetWindowDrawList()->AddLine(
              row_min, ImVec2(row_max.x, row_min.y),
              Packed(CurrentPalette().border),
              CurrentLayoutMetrics().geometry.border);
        }
        ImGui::TableSetColumnIndex(0);
        DrawStrongText(heading_font, profile.name);
        if (profile.is_default) {
          DrawBadge("Default");
        }
        ImGui::TableSetColumnIndex(1);
        detail::DrawSecondaryText("Physical bed");
        ImGui::Text("%.0f × %.0f mm", profile.bed_width_mm,
                    profile.bed_height_mm);
        ImGui::TableSetColumnIndex(2);
        const UsableBedSize usable = UsableSize(profile);
        detail::DrawSecondaryText("Usable area");
        ImGui::Text("%.0f × %.0f mm", usable.width_mm, usable.height_mm);
        ImGui::TableSetColumnIndex(3);
        if (Button({.id = "select",
                    .label = selected ? "Selected" : "Select",
                    .variant = ButtonVariant::Secondary,
                    .size = {.x = -1.0f, .y = 32.0f}})
                .activated) {
          state.draft.machines.selected_id = profile.id;
          RefreshSettingsDerivedState(state);
        }
        ImGui::PopID();
      }
      ImGui::EndTable();
    }
    ImGui::PopStyleVar();
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(5.0f));

  const bool editor_open = state.machine_editor.has_value();
  if (Button({.id = "new-machine",
              .label = "New",
              .variant = ButtonVariant::Primary,
              .size = {.x = 54.0f, .y = 32.0f}})
          .activated) {
    BeginNewMachine(state);
  }
  ImGui::SameLine();
  if (Button({.id = "edit-machine",
              .label = "Edit",
              .availability = {.enabled = !editor_open,
                               .reason =
                                   "Save or cancel Machine Information first"},
              .size = {.x = 54.0f, .y = 32.0f}})
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
              .size = {.x = 88.0f, .y = 32.0f}})
          .activated) {
    DuplicateSelectedMachine(state);
  }
  ImGui::SameLine(
      std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - Scale(91.0f)));
  if (Button({.id = "remove-machine",
              .label = "Remove",
              .variant = ButtonVariant::Destructive,
              .availability =
                  {.enabled = CanRemoveSelectedMachine(state),
                   .reason =
                       editor_open
                           ? "Save or cancel Machine Information first"
                           : "The default or final machine cannot be removed"},
              .size = {.x = 78.0f, .y = 32.0f}})
          .activated) {
    ImGui::OpenPopup("Remove machine profile");
  }
  if (state.remove_confirmation_open) {
    ImGui::OpenPopup("Remove machine profile");
    state.remove_confirmation_open = false;
  }
  if (editor_open) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ToImVec4(CurrentPalette().warning_background));
    if (ImGui::BeginChild("##machine-editor-retained",
                          ImVec2(0.0f, Scale(34.0f)))) {
      detail::DrawSecondaryText(
          "Machine Information is being kept until you save or cancel it.");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
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

void DrawMachineBedArea(SettingsGalleryState &state, ImFont *heading_font) {
  const MachineProfile *profile = SelectedMachine(state);
  if (profile == nullptr) {
    return;
  }
  const float heading_y = ImGui::GetCursorPosY();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  DrawKicker("SELECTED PROFILE");
  DrawStrongText(heading_font, profile->name);
  ImGui::PopStyleVar();
  if (profile->is_default) {
    const float width = ImGui::CalcTextSize("Default").x + Scale(10.0f);
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - width - Scale(12.0f),
                               heading_y + Scale(2.0f)));
    DrawBadge("Default");
  }
  ImGui::SetCursorPosY(heading_y + Scale(42.0f));
  const UsableBedSize usable = UsableSize(*profile);
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, Scale(0.0f)));
  if (ImGui::BeginTable("##machine-bed-summary", 2,
                        ImGuiTableFlags_SizingStretchProp |
                            ImGuiTableFlags_NoSavedSettings)) {
    ImGui::TableSetupColumn("Diagram", ImGuiTableColumnFlags_WidthStretch,
                            1.25f);
    ImGui::TableSetupColumn("Values", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableNextColumn();
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ToImVec4(CurrentPalette().surface_muted));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(10.0f), Scale(10.0f)));
    if (ImGui::BeginChild(
            "##bed-diagram-card", ImVec2(-Scale(6.0f), Scale(177.0f)),
            ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
      const ImVec2 diagram_min = ImGui::GetCursorScreenPos();
      const ImVec2 diagram_max(diagram_min.x + ImGui::GetContentRegionAvail().x,
                               diagram_min.y + Scale(120.0f));
      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      draw_list->AddRectFilled(diagram_min, diagram_max,
                               Packed(CurrentPalette().surface_raised));
      draw_list->AddRect(diagram_min, diagram_max,
                         Packed(CurrentPalette().border_strong), 0.0f, 0,
                         CurrentLayoutMetrics().geometry.focus_ring);
      const float inset_x = std::max(
          Scale(2.0f), static_cast<float>(profile->edge_insets_mm.left /
                                          profile->bed_width_mm) *
                           (diagram_max.x - diagram_min.x));
      const float inset_y =
          std::max(Scale(2.0f), static_cast<float>(profile->edge_insets_mm.top /
                                                   profile->bed_height_mm) *
                                    (diagram_max.y - diagram_min.y));
      DrawDashedRect(*draw_list,
                     ImVec2(diagram_min.x + inset_x, diagram_min.y + inset_y),
                     ImVec2(diagram_max.x - inset_x, diagram_max.y - inset_y),
                     Packed(CurrentPalette().warning));
      const std::string physical = std::format(
          "{:.0f} × {:.0f} mm", profile->bed_width_mm, profile->bed_height_mm);
      const std::string usable_text = std::format(
          "{:.0f} × {:.0f} mm usable", usable.width_mm, usable.height_mm);
      const ImVec2 physical_size = ImGui::CalcTextSize(physical.c_str());
      const ImVec2 usable_size = ImGui::CalcTextSize(usable_text.c_str());
      draw_list->AddText(
          ImVec2((diagram_min.x + diagram_max.x - physical_size.x) * 0.5f,
                 diagram_min.y + Scale(47.0f)),
          Packed(CurrentPalette().text_primary), physical.c_str());
      draw_list->AddText(
          ImVec2((diagram_min.x + diagram_max.x - usable_size.x) * 0.5f,
                 diagram_min.y + Scale(68.0f)),
          Packed(CurrentPalette().text_secondary), usable_text.c_str());
      draw_list->AddCircleFilled(ImVec2(diagram_min.x, diagram_max.y),
                                 Scale(4.0f), Packed(CurrentPalette().focus));
      ImGui::Dummy(ImVec2(0.0f, Scale(124.0f)));
      detail::DrawSecondaryTextWrapped(
          "Usable area is derived from the physical bed and four edge insets.");
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::TableNextColumn();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Scale(6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::BeginChild("##bed-values", ImVec2(0.0f, Scale(177.0f)),
                          ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar)) {
      const auto row = [heading_font](const char *label,
                                      const std::string &value,
                                      const bool emphasized = false) {
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 end(start.x + ImGui::GetContentRegionAvail().x,
                         start.y + Scale(44.0f));
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        if (emphasized) {
          draw_list->AddRectFilled(start, end,
                                   Packed(CurrentPalette().selection));
          draw_list->AddRectFilled(start, ImVec2(start.x + Scale(3.0f), end.y),
                                   Packed(CurrentPalette().focus));
        }
        if (start.y > ImGui::GetWindowPos().y) {
          draw_list->AddLine(start, ImVec2(end.x, start.y),
                             Packed(CurrentPalette().border),
                             CurrentLayoutMetrics().geometry.border);
        }
        draw_list->AddText(
            ImVec2(start.x + Scale(8.0f), start.y + Scale(14.0f)),
            Packed(CurrentPalette().text_secondary), label);
        const ImVec2 value_size = ImGui::CalcTextSize(value.c_str());
        if (heading_font != nullptr) {
          draw_list->AddText(
              heading_font, CurrentLayoutMetrics().typography.body_font_height,
              ImVec2(end.x - value_size.x - Scale(8.0f),
                     start.y + Scale(14.0f)),
              Packed(CurrentPalette().text_primary), value.c_str());
        } else {
          draw_list->AddText(ImVec2(end.x - value_size.x - Scale(8.0f),
                                    start.y + Scale(14.0f)),
                             Packed(CurrentPalette().text_primary),
                             value.c_str());
        }
        ImGui::Dummy(ImVec2(0.0f, Scale(44.0f)));
      };
      row("Physical bed",
          std::format("{:.0f} × {:.0f} mm", profile->bed_width_mm,
                      profile->bed_height_mm));
      row("Usable bed area",
          std::format("{:.0f} × {:.0f} mm", usable.width_mm, usable.height_mm),
          true);
      row("Origin", OriginLabel(profile->origin));
      row("Default material",
          profile->material_thickness_mm.has_value()
              ? std::format("{:.0f} mm", *profile->material_thickness_mm)
              : "Not set");
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(8.0f));
  DrawRule();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(1.0f));
  DrawStrongText(heading_font, "Edge insets");
  static constexpr std::array labels{"Top", "Right", "Bottom", "Left"};
  const std::array values{
      profile->edge_insets_mm.top, profile->edge_insets_mm.right,
      profile->edge_insets_mm.bottom, profile->edge_insets_mm.left};
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(Scale(4.0f), 0.0f));
  if (ImGui::BeginTable("##bed-insets", 4,
                        ImGuiTableFlags_SizingStretchSame |
                            ImGuiTableFlags_NoSavedSettings)) {
    for (std::size_t index = 0; index < labels.size(); ++index) {
      ImGui::TableNextColumn();
      ImGui::PushStyleColor(ImGuiCol_ChildBg,
                            ToImVec4(CurrentPalette().surface_muted));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(Scale(8.0f), Scale(7.0f)));
      if (ImGui::BeginChild(labels[index], ImVec2(0.0f, Scale(52.0f)))) {
        detail::DrawSecondaryText(labels[index]);
        DrawStrongText(heading_font, std::format("{:.0f} mm", values[index]));
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();
      ImGui::PopStyleColor();
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
}

void DrawMachinePresetBrowser(SettingsGalleryState &state,
                              ImFont *heading_font) {
  MachineEditorState &editor = *state.machine_editor;
  const float heading_y = ImGui::GetCursorPosY();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  DrawKicker("NEW PROFILE");
  DrawStrongText(heading_font, "Choose a machine preset");
  detail::DrawSecondaryText(
      std::format("{} exact-geometry presets · {} manufacturers",
                  kMachinePresets.size(), kMachinePresetManufacturers.size()));
  ImGui::PopStyleVar();
  const float heading_x = ImGui::GetCursorPosX();
  ImGui::SetCursorPos(ImVec2(heading_x + Scale(291.0f), heading_y));
  if (Button({.id = "preset-back",
              .label = "Back to information",
              .size = {.x = 148.0f, .y = 32.0f}})
          .activated) {
    CloseMachinePresetPicker(state);
    return;
  }
  ImGui::SetCursorPosY(heading_y + Scale(63.0f));
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(CurrentPalette().selection));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(Scale(10.0f), Scale(8.0f)));
  if (ImGui::BeginChild("##preset-help", ImVec2(0.0f, Scale(50.0f)))) {
    detail::DrawSecondaryTextWrapped(
        "Preset values remain editable. A machine profile is not created "
        "until you return and choose Save machine.");
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
  static constexpr std::array manufacturers{
      SelectOption{.id = "atomstack", .label = "AtomStack"},
      SelectOption{.id = "creality", .label = "Creality"},
      SelectOption{.id = "glowforge", .label = "Glowforge"},
      SelectOption{.id = "omtech", .label = "OMTech"},
      SelectOption{.id = "ortur", .label = "Ortur"},
      SelectOption{.id = "thunder-laser", .label = "Thunder Laser"},
      SelectOption{.id = "xtool", .label = "xTool"},
  };
  std::size_t manufacturer_index = 0;
  for (std::size_t index = 0; index < manufacturers.size(); ++index) {
    if (manufacturers[index].label == editor.preset_manufacturer) {
      manufacturer_index = index;
    }
  }
  const std::size_t shown = static_cast<std::size_t>(
      std::count_if(kMachinePresets.begin(), kMachinePresets.end(),
                    [&editor](const MachinePreset &preset) {
                      return preset.manufacturer == editor.preset_manufacturer;
                    }));
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginTable("##preset-filter", 2,
                        ImGuiTableFlags_SizingStretchProp |
                            ImGuiTableFlags_NoSavedSettings)) {
    ImGui::TableSetupColumn("Manufacturer", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed,
                            Scale(112.0f));
    ImGui::TableNextColumn();
    DrawStackedLabel("Manufacturer");
    const SelectResult manufacturer = Select({
        .id = "preset-manufacturer",
        .label = {},
        .options = manufacturers,
        .selected_index = manufacturer_index,
    });
    if (manufacturer.changed) {
      static_cast<void>(SelectMachinePresetManufacturer(
          state, manufacturers[manufacturer.selected_index].label));
    }
    ImGui::TableNextColumn();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(24.0f));
    ImGui::Text("%zu %s shown", shown, shown == 1 ? "preset" : "presets");
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,
                      ImVec2(Scale(4.0f), Scale(4.0f)));
  if (ImGui::BeginTable("##preset-cards", 2,
                        ImGuiTableFlags_SizingStretchSame |
                            ImGuiTableFlags_NoSavedSettings)) {
    for (const MachinePreset &preset : kMachinePresets) {
      if (preset.manufacturer != editor.preset_manufacturer) {
        continue;
      }
      ImGui::TableNextColumn();
      ImGui::PushID(preset.id.data());
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(Scale(8.0f), Scale(8.0f)));
      if (ImGui::BeginChild("##preset", ImVec2(0.0f, Scale(210.0f)),
                            ImGuiChildFlags_Borders,
                            ImGuiWindowFlags_NoScrollbar)) {
        const float card_y = ImGui::GetCursorPosY();
        ImGui::PushStyleVar(
            ImGuiStyleVar_ItemSpacing,
            ImVec2(ImGui::GetStyle().ItemSpacing.x, Scale(2.0f)));
        DrawStrongText(heading_font, preset.model);
        detail::DrawSecondaryText(preset.variant);
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(
            ImVec2(ImGui::GetWindowWidth() - Scale(101.0f), card_y));
        if (Button({.id = "use-preset",
                    .label = "Use preset",
                    .variant = ButtonVariant::Primary,
                    .size = {.x = 89.0f, .y = 32.0f}})
                .activated) {
          static_cast<void>(ApplyMachinePreset(state, preset.id));
        }
        ImGui::SetCursorPosY(card_y + Scale(44.0f));
        DrawRule();
        ImGui::SetCursorPosY(card_y + Scale(49.0f));
        detail::DrawSecondaryText("Origin");
        ImGui::SameLine();
        DrawStrongText(heading_font, OriginLabel(preset.origin));
        if (preset.requires_origin_confirmation) {
          ImGui::SetCursorPosY(card_y + Scale(67.0f));
          ImGui::PushStyleColor(ImGuiCol_Text,
                                ToImVec4(CurrentPalette().warning));
          ImGui::TextUnformatted("Confirmation required after selection");
          ImGui::PopStyleColor();
        }
        ImGui::SetCursorPosY(card_y + Scale(preset.requires_origin_confirmation
                                                ? 88.0f
                                                : 76.0f));
        if (ImGui::BeginTable("##measurements", 2,
                              ImGuiTableFlags_SizingStretchSame |
                                  ImGuiTableFlags_NoSavedSettings)) {
          for (const char *label : {"Physical bed", "Working area"}) {
            ImGui::TableNextColumn();
            detail::DrawSecondaryText(label);
            DrawStrongText(heading_font, std::format("{:.0f} × {:.0f} mm",
                                                     preset.bed_width_mm,
                                                     preset.bed_height_mm));
            detail::DrawSecondaryText(std::format("{:.3f} × {:.3f} in",
                                                  preset.bed_width_mm / 25.4,
                                                  preset.bed_height_mm / 25.4));
          }
          ImGui::EndTable();
        }
        DrawRule();
        detail::DrawSecondaryText("Margins");
        ImGui::SameLine();
        DrawStrongText(heading_font, "None");
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
}

void DrawMachineInformation(SettingsGalleryState &state, ImFont *heading_font) {
  if (!state.machine_editor.has_value()) {
    const float available = ImGui::GetContentRegionAvail().y;
    ImGui::Dummy(ImVec2(0.0f, std::max(Scale(52.0f), available * 0.25f)));
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - Scale(44.0f)) * 0.5f);
    DrawBadge("＋");
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() -
                          ImGui::CalcTextSize("No machine is open").x) *
                         0.5f);
    ImGui::TextUnformatted("No machine is open");
    const char *help =
        "Choose New or Edit from Profiles to open machine information.";
    ImGui::SetCursorPosX(
        (ImGui::GetWindowWidth() - ImGui::CalcTextSize(help).x) * 0.5f);
    detail::DrawSecondaryText(help);
    return;
  }
  if (state.machine_editor->preset_picker_open) {
    DrawMachinePresetBrowser(state, heading_font);
    return;
  }

  MachineEditorState &editor = *state.machine_editor;
  MachineProfile &profile = editor.draft;
  const bool new_machine = editor.mode == MachineEditorMode::New;
  const std::size_t error_count = editor.errors.size();
  const float heading_y = ImGui::GetCursorPosY();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  DrawKicker(new_machine ? "NEW PROFILE" : "EDITING SELECTED PROFILE");
  const std::string title =
      new_machine ? "New machine" : "Edit " + profile.name;
  DrawStrongText(heading_font, title);
  detail::DrawSecondaryText(
      error_count > 0 ? std::to_string(error_count) +
                            (error_count == 1 ? " field needs attention"
                                              : " fields need attention")
      : editor.dirty  ? "Ready to save"
                      : "No local changes");
  ImGui::PopStyleVar();
  const float action_width = new_machine ? Scale(361.0f) : Scale(235.0f);
  ImGui::SetCursorPos(
      ImVec2(ImGui::GetWindowWidth() - action_width - Scale(12.0f), heading_y));
  if (new_machine) {
    if (Button({.id = "choose-preset",
                .label = "Choose preset…",
                .size = {.x = 126.0f, .y = 32.0f}})
            .activated) {
      static_cast<void>(OpenMachinePresetPicker(state));
      return;
    }
    ImGui::SameLine();
  }
  if (Button({.id = "cancel-machine-editor",
              .label = "Cancel editing",
              .size = {.x = 115.0f, .y = 32.0f}})
          .activated) {
    CancelMachineEditor(state);
    return;
  }
  ImGui::SameLine();
  if (Button({.id = "save-machine",
              .label = "Save machine",
              .variant = ButtonVariant::Primary,
              .availability = {.enabled = error_count == 0,
                               .reason =
                                   "Resolve Machine Information errors first"},
              .size = {.x = 112.0f, .y = 32.0f}})
          .activated) {
    static_cast<void>(SaveMachineEditor(state));
    return;
  }
  ImGui::SetCursorPosY(heading_y + Scale(58.0f));

  if (editor.origin_confirmation_required && !editor.origin_confirmed) {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(3.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ToImVec4(CurrentPalette().warning_background));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(10.0f), Scale(8.0f)));
    if (ImGui::BeginChild("##origin-confirmation",
                          ImVec2(0.0f, Scale(68.0f)))) {
      const float warning_y = ImGui::GetCursorPosY();
      ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(CurrentPalette().warning));
      ImGui::TextUnformatted("Confirm the suggested origin");
      ImGui::PopStyleColor();
      ImGui::TextWrapped("This preset suggests %s. Verify the machine setup or "
                         "choose another corner.",
                         OriginLabel(profile.origin));
      ImGui::SetCursorPos(
          ImVec2(ImGui::GetWindowWidth() - Scale(164.0f), warning_y));
      const std::string confirm =
          std::string("Confirm ") + OriginLabel(profile.origin);
      if (Button({.id = "confirm-origin",
                  .label = confirm,
                  .size = {.x = 152.0f, .y = 32.0f}})
              .activated) {
        ConfirmMachinePresetOrigin(state);
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - Scale(3.0f));
    if (state.request_machine_confirmation_scroll) {
      state.request_machine_confirmation_scroll = false;
    }
  }

  DrawStackedLabel("Name");
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginTable("##machine-name-row", 2,
                        ImGuiTableFlags_SizingStretchProp |
                            ImGuiTableFlags_NoSavedSettings)) {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthFixed,
                            Scale(117.0f));
    ImGui::TableNextColumn();
    const TextInputResult name = TextInput({
        .id = "machine-name",
        .label = {},
        .value = profile.name,
        .placeholder = "Required",
        .validation = MachineEditorValidation(state, "name"),
    });
    if (name.changed) {
      profile.name = name.value;
      RefreshSettingsDerivedState(state);
    }
    ImGui::TableNextColumn();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Scale(8.0f));
    if (Button(
            {.id = "make-default",
             .label = profile.is_default ? "Default machine" : "Make default",
             .availability = {.enabled = !profile.is_default,
                              .reason = "This profile is already the default"},
             .size = {.x = -1.0f, .y = 32.0f}})
            .activated) {
      profile.is_default = true;
      RefreshSettingsDerivedState(state);
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(Scale(6.0f), 0.0f));
  if (ImGui::BeginTable("##machine-editor-grid", 2,
                        ImGuiTableFlags_SizingStretchSame |
                            ImGuiTableFlags_NoSavedSettings)) {
    ImGui::TableNextColumn();
    DrawSectionHeading("Physical bed", false, heading_font);
    if (ImGui::BeginTable("##physical-size", 2,
                          ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_NoSavedSettings)) {
      ImGui::TableNextColumn();
      DrawMachineEditorNumber(state, "bed-width", "Bed width",
                              profile.bed_width_mm, "bed_width_mm");
      ImGui::TableNextColumn();
      DrawMachineEditorNumber(state, "bed-height", "Bed height",
                              profile.bed_height_mm, "bed_height_mm");
      ImGui::EndTable();
    }
    const UsableBedSize usable = UsableSize(profile);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ToImVec4(CurrentPalette().selection));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(8.0f), Scale(8.0f)));
    const bool valid = usable.width_mm > 0.0 && usable.height_mm > 0.0;
    const float summary_gap =
        editor.origin_confirmation_required && !editor.origin_confirmed ? 6.0f
        : valid                                                         ? 3.0f
                                                                        : 4.0f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(summary_gap));
    if (ImGui::BeginChild("##derived-usable",
                          ImVec2(0.0f, Scale(valid ? 33.0f : 58.0f)))) {
      const float row_y = ImGui::GetCursorPosY();
      detail::DrawSecondaryText("Usable after insets");
      const std::string value =
          valid ? std::format("{:.0f} × {:.0f} mm", usable.width_mm,
                              usable.height_mm)
                : "Complete valid bed dimensions";
      ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() -
                                     ImGui::CalcTextSize(value.c_str()).x -
                                     Scale(8.0f),
                                 row_y + Scale(valid ? 0.0f : 23.0f)));
      DrawStrongText(heading_font, value);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::TableNextColumn();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(7.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ToImVec4(CurrentPalette().surface_muted));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(8.0f), Scale(8.0f)));
    if (ImGui::BeginChild("##machine-insets", ImVec2(0.0f, Scale(168.0f)),
                          ImGuiChildFlags_Borders)) {
      DrawStackedLabel("Edge insets");
      if (ImGui::BeginTable("##insets", 2,
                            ImGuiTableFlags_SizingStretchSame |
                                ImGuiTableFlags_NoSavedSettings)) {
        const std::array fields{
            std::tuple{"inset-top", "Top", &profile.edge_insets_mm.top,
                       "edge_insets_mm.top"},
            std::tuple{"inset-right", "Right", &profile.edge_insets_mm.right,
                       "edge_insets_mm.right"},
            std::tuple{"inset-bottom", "Bottom", &profile.edge_insets_mm.bottom,
                       "edge_insets_mm.bottom"},
            std::tuple{"inset-left", "Left", &profile.edge_insets_mm.left,
                       "edge_insets_mm.left"},
        };
        for (const auto &[id, label, value, path] : fields) {
          ImGui::TableNextColumn();
          DrawMachineEditorNumber(state, id, label, *value, path);
        }
        ImGui::EndTable();
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(6.0f));

  static constexpr std::array origins{
      SelectOption{.id = "bottom-left", .label = "Bottom Left"},
      SelectOption{.id = "bottom-right", .label = "Bottom Right"},
      SelectOption{.id = "top-left", .label = "Top Left"},
      SelectOption{.id = "top-right", .label = "Top Right"},
  };
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(Scale(6.0f), 0.0f));
  if (ImGui::BeginTable("##machine-profile-fields", 2,
                        ImGuiTableFlags_SizingStretchSame |
                            ImGuiTableFlags_NoSavedSettings)) {
    ImGui::TableNextColumn();
    DrawStackedLabel("Origin location");
    const SelectResult origin = Select({
        .id = "origin",
        .label = {},
        .options = origins,
        .selected_index = OriginIndex(profile.origin),
        .validation = MachineEditorValidation(state, "origin_confirmation"),
    });
    if (origin.changed) {
      SetMachineEditorOrigin(state, OriginFromIndex(origin.selected_index));
    }
    ImGui::TableNextColumn();
    if (profile.material_thickness_mm.has_value()) {
      DrawMachineEditorNumber(
          state, "material-thickness", "Default material thickness (optional)",
          *profile.material_thickness_mm, "material_thickness_mm");
    } else {
      const TextInputResult thickness = DrawStackedTextInput({
          .id = "material-thickness",
          .label = "Default material thickness (optional)",
          .value = {},
          .placeholder = "Optional",
      });
      if (thickness.changed && !thickness.value.empty()) {
        double parsed = 0.0;
        const auto [end, error] = std::from_chars(
            thickness.value.data(),
            thickness.value.data() + thickness.value.size(), parsed);
        if (error == std::errc{} &&
            end == thickness.value.data() + thickness.value.size()) {
          profile.material_thickness_mm = parsed;
          RefreshSettingsDerivedState(state);
        }
      }
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
}

void DrawMachines(detail::UiAssetAtlas &assets, SettingsGalleryState &state) {
  DrawPageHeader(assets, "Machines",
                 "Define named machine beds once and reuse them throughout 3D "
                 "and Canvas.",
                 0.0f);
  const std::string information_label =
      state.machine_editor.has_value() && state.machine_editor->dirty
          ? "Machine Information •"
          : "Machine Information";
  const std::array machine_tabs{
      ChoiceSpec{.id = "profiles", .label = "Profiles"},
      ChoiceSpec{.id = "bed-area", .label = "Usable Bed Area"},
      ChoiceSpec{.id = "information", .label = information_label},
  };
  const float tab_width = ImGui::GetContentRegionAvail().x / CurrentUiScale();
  const TabSetResult tabs = TabSet({
      .id = "machine-tabs",
      .tabs = machine_tabs,
      .selected_index = static_cast<std::size_t>(state.active_machine_tab),
      .width = tab_width,
      .draw_panel =
          [&state,
           heading_font = assets.heading_font()](const std::size_t index) {
            state.active_machine_tab = static_cast<MachineSettingsTab>(index);
            float panel_height = Scale(268.0f);
            if (state.active_machine_tab == MachineSettingsTab::BedArea) {
              panel_height = Scale(346.0f);
            } else if (state.active_machine_tab ==
                       MachineSettingsTab::Information) {
              panel_height = state.machine_editor.has_value() &&
                                     state.machine_editor->preset_picker_open
                                 ? Scale(900.0f)
                                 : Scale(650.0f);
            }
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                                ImVec2(Scale(16.0f), Scale(12.0f)));
            if (ImGui::BeginChild("##machine-tab-panel",
                                  ImVec2(0.0f, panel_height),
                                  ImGuiChildFlags_Borders,
                                  ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse)) {
              switch (state.active_machine_tab) {
              case MachineSettingsTab::Profiles:
                DrawMachineProfiles(state, heading_font);
                break;
              case MachineSettingsTab::BedArea:
                DrawMachineBedArea(state, heading_font);
                break;
              case MachineSettingsTab::Information:
                DrawMachineInformation(state, heading_font);
                break;
              }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
          },
  });
  if (tabs.changed) {
    state.active_machine_tab =
        static_cast<MachineSettingsTab>(tabs.selected_index);
  }
}

void DrawLicense(detail::UiAssetAtlas &assets, SettingsGalleryState &state) {
  DrawPageHeader(
      assets, "License",
      "Product activation is account-level state and applies immediately.",
      0.0f);
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(12.0f));
  const bool active = state.license.status == LicenseStatus::Active;
  const ColorRgba status_color =
      active ? CurrentPalette().success : CurrentPalette().warning;
  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        ToImVec4(CurrentPalette().surface_muted));
  ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(CurrentPalette().border));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(Scale(18.0f), Scale(18.0f)));
  if (ImGui::BeginChild("##license-status", ImVec2(0.0f, Scale(88.0f)),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar)) {
    const ImVec2 window = ImGui::GetWindowPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(
        window,
        ImVec2(window.x + Scale(4.0f), window.y + ImGui::GetWindowHeight()),
        Packed(status_color));
    const ImVec2 mark_min = ImGui::GetCursorScreenPos();
    const ImVec2 mark_max(mark_min.x + Scale(42.0f), mark_min.y + Scale(42.0f));
    draw_list->AddCircleFilled(
        ImVec2((mark_min.x + mark_max.x) * 0.5f,
               (mark_min.y + mark_max.y) * 0.5f),
        Scale(21.0f),
        Packed(active ? CurrentPalette().success_background
                      : CurrentPalette().warning_background));
    static_cast<void>(assets.DrawIcon(
        active ? "success" : "license", steppenface::UiIconSize::Small16,
        {.minimum = {.x = mark_min.x + Scale(11.0f),
                     .y = mark_min.y + Scale(11.0f)},
         .maximum = {.x = mark_max.x - Scale(11.0f),
                     .y = mark_max.y - Scale(11.0f)}},
        status_color));
    const float text_x = mark_max.x + Scale(12.0f);
    draw_list->AddText(ImVec2(text_x, mark_min.y - Scale(2.0f)),
                       Packed(CurrentPalette().text_secondary),
                       "ACTIVATION STATUS");
    const char *title =
        active ? "Export Face is activated" : "Export Face is not activated";
    if (assets.heading_font() != nullptr) {
      draw_list->AddText(assets.heading_font(),
                         CurrentLayoutMetrics().typography.body_font_height,
                         ImVec2(text_x, mark_min.y + Scale(15.0f)),
                         Packed(CurrentPalette().text_primary), title);
    } else {
      draw_list->AddText(ImVec2(text_x, mark_min.y + Scale(15.0f)),
                         Packed(CurrentPalette().text_primary), title);
    }
    const std::string description =
        active ? std::format("Licensed to {} · {}", state.license.owner,
                             state.license.masked_key)
               : "Enter a product key to enable licensed builds on this "
                 "computer.";
    draw_list->AddText(ImVec2(text_x, mark_min.y + Scale(33.0f)),
                       Packed(CurrentPalette().text_secondary),
                       description.c_str());
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);
  if (active) {
    if (Button({
                   .id = "deactivate-license",
                   .label = "Deactivate this computer",
                   .variant = ButtonVariant::Destructive,
               })
            .activated) {
      DeactivateLicense(state);
    }
    detail::DrawSecondaryTextWrapped(
        "Deactivation does not change staged preferences elsewhere in this "
        "window.");
    return;
  }

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - Scale(6.0f));
  DrawSectionHeading("Activate a product key", true, assets.heading_font());
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - Scale(2.0f));
  DrawStackedLabel("Product key");
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginTable("##license-key-row", 2,
                        ImGuiTableFlags_SizingStretchProp |
                            ImGuiTableFlags_NoSavedSettings)) {
    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                            Scale(91.0f));
    ImGui::TableNextColumn();
    const TextInputResult key = TextInput({
        .id = "product-key",
        .label = {},
        .value = state.license.key,
        .placeholder = "XXXX-XXXX-XXXX-XXXX",
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
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Scale(8.0f));
    if (Button({.id = "activate-license",
                .label = "Activate",
                .variant = ButtonVariant::Primary,
                .size = {.x = -1.0f, .y = 32.0f}})
            .activated) {
      static_cast<void>(ActivateLicense(state));
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(2.0f));
  detail::DrawSecondaryTextWrapped(
      "Activation requires a valid product key. This mockup does not contact "
      "a license server.");
}

void DrawLegal(detail::UiAssetAtlas &assets) {
  DrawPageHeader(assets, "Legal",
                 "Application and third-party notices for Export Face 0.1.0.");
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - Scale(2.0f));
  DrawSectionHeading("Export Face", false, assets.heading_font());
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - Scale(2.0f));
  detail::DrawSecondaryTextWrapped(
      "Copyright © 2026 SonderMill. All rights reserved.");
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(10.0f));
  DrawSectionHeading("Third-party software", true, assets.heading_font());
  static constexpr std::array notices{
      std::pair{"Open CASCADE Technology", "GNU LGPL 2.1 with OCCT exception"},
      std::pair{"Dear ImGui", "MIT License"},
      std::pair{"Clipper2", "Boost Software License 1.0"},
      std::pair{"Boost", "Boost Software License 1.0"},
  };
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,
                      ImVec2(Scale(8.0f), Scale(8.0f)));
  if (ImGui::BeginTable("##legal-notices", 3,
                        ImGuiTableFlags_BordersOuter |
                            ImGuiTableFlags_BordersH |
                            ImGuiTableFlags_SizingStretchProp |
                            ImGuiTableFlags_NoSavedSettings)) {
    ImGui::TableSetupColumn("Component", ImGuiTableColumnFlags_WidthStretch,
                            1.3f);
    ImGui::TableSetupColumn("License", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                            Scale(112.0f));
    for (const auto &[component, license] : notices) {
      ImGui::TableNextRow(ImGuiTableRowFlags_None, Scale(46.0f));
      ImGui::TableNextColumn();
      DrawStrongText(assets.heading_font(), component);
      ImGui::TableNextColumn();
      detail::DrawSecondaryText(license);
      ImGui::TableNextColumn();
      ImGui::PushID(component);
      ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(CurrentPalette().focus));
      if (Button({.id = "view-license",
                  .label = "View license",
                  .variant = ButtonVariant::Tertiary,
                  .size = {.x = -1.0f, .y = 28.0f}})
              .activated) {
        ImGui::OpenPopup("License text");
      }
      ImGui::PopStyleColor();
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
  ImGui::PopStyleVar();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(3.0f));
  detail::DrawSecondaryTextWrapped(
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
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  for (const Section &section : sections) {
    ImGui::PushID(section.label);
    const bool active = state.active_section == section.id;
    const bool activated = ImGui::InvisibleButton(
        "##section", ImVec2(-1.0f, Scale(54.0f)), ImGuiButtonFlags_EnableNav);
    const InteractionResult interaction = detail::CaptureInteraction();
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    if (active || interaction.hovered) {
      draw_list->AddRectFilled(
          minimum, maximum,
          Packed(active ? palette.selection : palette.control_hover),
          metrics.geometry.control_radius);
    }
    if (active) {
      const float center_y = (minimum.y + maximum.y) * 0.5f;
      draw_list->AddRectFilled(
          ImVec2(ImGui::GetWindowPos().x, center_y - Scale(15.0f)),
          ImVec2(ImGui::GetWindowPos().x + Scale(3.0f),
                 center_y + Scale(15.0f)),
          Packed(palette.focus));
    }
    const float icon_size = Scale(16.0f);
    const float icon_left = minimum.x + Scale(10.0f);
    const float icon_top =
        minimum.y + (maximum.y - minimum.y - icon_size) * 0.5f;
    static_cast<void>(assets.DrawIcon(
        section.icon, steppenface::UiIconSize::Small16,
        {.minimum = {.x = icon_left, .y = icon_top},
         .maximum = {.x = icon_left + icon_size, .y = icon_top + icon_size}},
        active ? palette.focus : palette.text_secondary));
    const ImVec2 label_size = ImGui::CalcTextSize(section.label);
    draw_list->AddText(
        ImVec2(minimum.x + Scale(37.0f),
               std::floor((minimum.y + maximum.y - label_size.y) * 0.5f)),
        Packed(active ? palette.focus : palette.text_secondary), section.label);
    detail::DrawFocusRing(interaction);
    if (activated) {
      state.active_section = section.id;
    }
    ImGui::PopID();
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
  const bool visible = ImGui::Begin(
      "System Settings##gallery", &open,
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
          ImGuiWindowFlags_NoScrollWithMouse);
  const bool escape =
      ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
      ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (visible) {
    bool close_requested = false;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(palette.surface_raised));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::BeginChild("##settings-title-bar",
                          ImVec2(0.0f, metrics.settings.title_bar_height), 0,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse)) {
      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      const ImVec2 window = ImGui::GetWindowPos();
      draw_list->AddText(
          ImVec2(window.x + Scale(16.0f), window.y + Scale(6.0f)),
          Packed(palette.text_secondary), "EXPORT FACE");
      if (assets.heading_font() != nullptr) {
        draw_list->AddText(
            assets.heading_font(),
            metrics.typography.settings_title_font_height,
            ImVec2(window.x + Scale(16.0f), window.y + Scale(22.0f)),
            Packed(palette.text_primary), "System Settings");
      } else {
        draw_list->AddText(
            ImVec2(window.x + Scale(16.0f), window.y + Scale(22.0f)),
            Packed(palette.text_primary), "System Settings");
      }
      ImGui::SetCursorScreenPos(
          ImVec2(window.x + ImGui::GetWindowWidth() - Scale(40.0f),
                 window.y + Scale(8.0f)));
      close_requested = ImGui::InvisibleButton(
          "##settings-close", ImVec2(Scale(32.0f), Scale(32.0f)),
          ImGuiButtonFlags_EnableNav);
      const InteractionResult close_interaction = detail::CaptureInteraction();
      if (close_interaction.hovered) {
        draw_list->AddRectFilled(
            ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
            Packed(palette.control_hover), metrics.geometry.control_radius);
      }
      static_cast<void>(assets.DrawIcon(
          "failure", steppenface::UiIconSize::Small16,
          {.minimum = {.x = ImGui::GetItemRectMin().x + Scale(8.0f),
                       .y = ImGui::GetItemRectMin().y + Scale(8.0f)},
           .maximum = {.x = ImGui::GetItemRectMax().x - Scale(8.0f),
                       .y = ImGui::GetItemRectMax().y - Scale(8.0f)}},
          palette.text_secondary));
      detail::DrawFocusRing(close_interaction);
      draw_list->AddLine(
          ImVec2(window.x,
                 window.y + ImGui::GetWindowHeight() - metrics.geometry.border),
          ImVec2(window.x + ImGui::GetWindowWidth(),
                 window.y + ImGui::GetWindowHeight() - metrics.geometry.border),
          Packed(palette.border), metrics.geometry.border);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() -
                         ImGui::GetStyle().ItemSpacing.y);

    const float footer_height = Scale(52.0f);
    const float navigation_width = Scale(152.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(palette.surface_muted));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(8.0f), Scale(8.0f)));
    if (ImGui::BeginChild("##settings-navigation",
                          ImVec2(navigation_width, 0.0f),
                          ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse)) {
      DrawSettingsNavigation(assets, state);
      const float border_x = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() -
                             metrics.geometry.border;
      ImGui::GetWindowDrawList()->AddLine(
          ImVec2(border_x, ImGui::GetWindowPos().y),
          ImVec2(border_x, ImGui::GetWindowPos().y + ImGui::GetWindowHeight()),
          Packed(palette.border), metrics.geometry.border);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::BeginChild("##settings-right", ImVec2(0.0f, 0.0f), 0,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse)) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(Scale(24.0f), Scale(24.0f)));
      const float page_width =
          state.active_section == SettingsSection::Machines
              ? 0.0f
              : std::min(Scale(680.0f), ImGui::GetContentRegionAvail().x);
      if (ImGui::BeginChild("##settings-page",
                            ImVec2(page_width, -footer_height),
                            ImGuiChildFlags_AlwaysUseWindowPadding)) {
        DrawSettingsPage(assets, gallery);
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();

      ImGui::SetCursorPosY(ImGui::GetCursorPosY() -
                           ImGui::GetStyle().ItemSpacing.y);

      ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(palette.surface_raised));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(Scale(16.0f), Scale(10.0f)));
      if (ImGui::BeginChild("##settings-footer", ImVec2(0.0f, footer_height),
                            ImGuiChildFlags_AlwaysUseWindowPadding,
                            ImGuiWindowFlags_NoScrollbar)) {
        ImGui::GetWindowDrawList()->AddLine(
            ImGui::GetWindowPos(),
            ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(),
                   ImGui::GetWindowPos().y),
            Packed(palette.border), metrics.geometry.border);
        const std::string status =
            state.machine_editor.has_value()
                ? "Save or cancel Machine Information before applying"
            : !state.errors.empty()
                ? std::to_string(state.errors.size()) +
                      (state.errors.size() == 1 ? " field needs attention"
                                                : " fields need attention")
            : state.dirty ? "Unapplied changes"
                          : "Settings are up to date";
        detail::DrawSecondaryText(status);
        const float button_width = Scale(176.0f);
        ImGui::SameLine(std::max(ImGui::GetCursorPosX(),
                                 ImGui::GetWindowWidth() - button_width));
        if (Button({.id = "settings-cancel",
                    .label = "Cancel",
                    .size = {.x = 80.0f, .y = 32.0f}})
                .activated) {
          DiscardSettings(state);
          gallery.theme = ResolveSettingsTheme(state.applied.appearance.theme,
                                               state.system_theme);
          state.window_open = false;
        }
        ImGui::SameLine();
        if (Button({
                       .id = "settings-apply",
                       .label = "Apply",
                       .variant = ButtonVariant::Primary,
                       .availability =
                           {
                               .enabled = !state.machine_editor.has_value() &&
                                          state.dirty && state.errors.empty(),
                               .reason = state.machine_editor.has_value()
                                             ? "Save or cancel Machine "
                                               "Information before Apply"
                                         : !state.errors.empty()
                                             ? "Resolve validation errors "
                                               "before Apply"
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
      ImGui::EndChild();
      ImGui::PopStyleVar();
      ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
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
