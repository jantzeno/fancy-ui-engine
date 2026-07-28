#pragma once

#include "gallery_settings_model.hpp"
#include "gallery_state_model.hpp"

#include "fancy_ui/components/fields.hpp"
#include "fancy_ui/shell/application.hpp"
#include "fancy_ui/theme.hpp"

#include <array>
#include <string>

namespace fancy_ui::detail {
class UiAssetAtlas;
}

namespace fancy_ui::gallery {

struct ShellGalleryState {
  shell::ApplicationShellState layout{
      .explorer_visible = true,
      .inspector_visible = true,
      .operation_tray_visible = true,
  };
  std::string object_name = "Face plate";
  double spacing_mm = 8.0;
  std::size_t material_index = 0;
  int hours = 0;
  int minutes = 5;
  ToggleState enabled = ToggleState::On;
  ToggleState visible = ToggleState::On;
  float explode_percent = 38.0f;
  int rotation_count = 8;
  ColorRgba display_color{
      .red = 0.27f,
      .green = 0.58f,
      .blue = 0.97f,
  };
  ColorPickerState color_picker;
  std::string feedback = "Inspector controls are live.";
  OperationPresentationState operation{
      .expanded = true,
  };
};

struct GalleryState {
  GalleryTab active_tab = GalleryTab::Components;
  ResolvedTheme theme = ResolvedTheme::Dark;
  float scale = 1.0f;
  double spacing = 8.0;
  std::size_t rotation_option = 0;
  int hours = 0;
  int minutes = 5;
  float explode = 38.0f;
  int rotations = 8;
  std::string button_feedback = "Activate a button to inspect its result.";
  bool availability_selected = true;
  ToggleState checkbox = ToggleState::On;
  ToggleState checkbox_off = ToggleState::Off;
  ToggleState checkbox_mixed = ToggleState::Mixed;
  ToggleState visible = ToggleState::On;
  ToggleState guides_visible = ToggleState::Off;
  ToggleState selection_visible = ToggleState::Mixed;
  ToggleState enabled = ToggleState::On;
  ToggleState locked = ToggleState::Off;
  ToggleState margins = ToggleState::Mixed;
  ToggleState mixed_visibility = ToggleState::Mixed;

  bool assembly_expanded = true;
  bool part_expanded = true;
  std::array<bool, 3> tree_selected{true, false, false};
  int tree_selection_anchor = 0;
  std::array<ToggleState, 2> tree_visibility{ToggleState::Off, ToggleState::On};
  ColorRgba part_color{
      .red = 0.27f,
      .green = 0.58f,
      .blue = 0.97f,
  };
  ColorPickerState tree_color_picker;
  int tree_action_row = -1;
  std::string tree_feedback = "Select a row or use an inline action.";

  bool show_issue_labels = true;
  std::array<bool, 3> issue_groups_expanded{false, false, false};
  std::array<bool, 5> issue_selected{};
  int issue_selection_anchor = -1;
  std::array<ToggleState, 5> issue_visibility{
      ToggleState::On, ToggleState::Off, ToggleState::On,
      ToggleState::On, ToggleState::Off,
  };
  std::string issue_feedback = "Expand a group and select an issue.";

  bool bed_color_mixed = true;
  ColorRgba bed_color{
      .red = 0.27f,
      .green = 0.58f,
      .blue = 0.97f,
  };
  ColorPickerState bed_color_picker;

  ColorRgba preview_picker_color{
      .red = 0.64f,
      .green = 0.44f,
      .blue = 0.97f,
      .alpha = 0.78f,
  };
  ColorPickerState preview_picker;
  ColorRgba compact_picker_color{
      .red = 0.82f,
      .green = 0.60f,
      .blue = 0.13f,
      .alpha = 0.86f,
  };
  ColorPickerState compact_picker;

  std::array<OperationPresentationState, kOperationSampleCount>
      operation_states = DefaultOperationPresentationStates();
  std::array<StatusZoomPresentationState, kStatusSampleCount>
      status_zoom_states = DefaultStatusZoomPresentationStates();
  ShellGalleryState shell;
  SettingsGalleryState settings = DefaultSettingsGalleryState();
};

void DrawComponentGallery(detail::UiAssetAtlas &assets, GalleryState &state);
void DrawApplicationShellGallery(detail::UiAssetAtlas &assets,
                                 GalleryState &state);
void DrawSettingsGallery(detail::UiAssetAtlas &assets, GalleryState &state);
void DrawSettingsGalleryWindow(detail::UiAssetAtlas &assets,
                               GalleryState &state);
void DrawOperationStateGallery(detail::UiAssetAtlas &assets,
                               GalleryState &state);
void DrawStatusBarStateGallery(detail::UiAssetAtlas &assets,
                               GalleryState &state);
void DrawHierarchySample(detail::UiAssetAtlas &assets, GalleryState &state);
void DrawShellOperationTray(detail::UiAssetAtlas &assets, GalleryState &state);
void DrawShellOperationStrip(detail::UiAssetAtlas &assets,
                             GalleryState &state);
void DrawShellStatusBar(detail::UiAssetAtlas &assets, GalleryState &state);

} // namespace fancy_ui::gallery
