#pragma once

#include "gallery_settings_model.hpp"
#include "gallery_state_model.hpp"

#include "fancy_ui/components/color_picker_types.hpp"
#include "fancy_ui/components/context_menu.hpp"
#include "fancy_ui/components/grain_direction_gizmo.hpp"
#include "fancy_ui/components/modeless_window.hpp"
#include "fancy_ui/components/renamable_select.hpp"
#include "fancy_ui/components/status_zoom_popover.hpp"
#include "fancy_ui/shell/application.hpp"
#include "fancy_ui/steppenface/application_view.hpp"
#include "fancy_ui/theme.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace fancy_ui::detail {
class UiAssetAtlas;
}

namespace fancy_ui::gallery {

enum class ComponentCaptureVariant {
  Default,
  ContextMenuOpen,
  StatusZoomOpen,
  Renaming,
  MultiselectOpen,
  ModelessWindowOpen,
  GrainBed,
  GrainPart,
  GrainPaired,
  GrainDragging,
  GrainLocked,
};

struct GalleryCanvasToolbarState {
  steppenface::SelectionScope selection_scope =
      steppenface::SelectionScope::Canvas;
  steppenface::SelectionTool selection_tool =
      steppenface::SelectionTool::Pointer;
  bool grid_visible = true;
  double grid_spacing_mm = 10.0;
  bool snap_guides = true;
  bool snap_major_grid = true;
  bool snap_minor_grid = false;
  bool snap_margins = false;
};

struct GalleryModelBedToolbarState {
  int id = 0;
  std::string name;
  int grid_spacing_mm = 25;
  bool snap_to_grid = true;
};

struct GalleryModelToolbarState {
  steppenface::SelectionTool selection_tool =
      steppenface::SelectionTool::Pointer;
  std::string grid_target = "all";
  std::array<GalleryModelBedToolbarState, 2> beds{
      GalleryModelBedToolbarState{.id = 1, .name = "Bed 1"},
      GalleryModelBedToolbarState{.id = 2, .name = "Bed 2"},
  };
};

struct ShellGalleryState {
  shell::ApplicationShellState layout{
      .explorer_visible = true,
      .inspector_visible = true,
      .operation_tray_visible = true,
  };
  steppenface::WorkspaceKind active_workspace =
      steppenface::WorkspaceKind::Canvas;
  bool has_selection = true;
  bool has_model = true;
  bool has_assigned_selection = true;
  bool can_convert_to_partbed = true;
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
  GalleryCanvasToolbarState canvas_toolbar;
  GalleryModelToolbarState model_toolbar;
  OperationPresentationState operation{
      .expanded = true,
  };
};

struct HierarchyCardState {
  std::array<bool, 7> expanded{true, true, true, true, true, true, true};
  std::array<bool, 7> selected{};
  int selection_anchor = -1;
  std::array<ToggleState, 7> visibility{
      ToggleState::On, ToggleState::On, ToggleState::On, ToggleState::On,
      ToggleState::On, ToggleState::On, ToggleState::On,
  };
  std::array<ColorRgba, 7> colors{};
  ColorPickerState color_picker;
  int color_row = 0;
  int action_row = 0;
  bool request_actions = false;
};

[[nodiscard]] inline std::array<HierarchyCardState, 3>
DefaultHierarchyCardStates() {
  constexpr ColorRgba purple{
      .red = 0xa3 / 255.0f,
      .green = 0x71 / 255.0f,
      .blue = 0xf7 / 255.0f,
  };
  constexpr ColorRgba blue{
      .red = 0x2f / 255.0f,
      .green = 0x81 / 255.0f,
      .blue = 0xf7 / 255.0f,
  };
  constexpr ColorRgba green{
      .red = 0x3f / 255.0f,
      .green = 0xb9 / 255.0f,
      .blue = 0x50 / 255.0f,
  };
  constexpr ColorRgba amber{
      .red = 0xd2 / 255.0f,
      .green = 0x99 / 255.0f,
      .blue = 0x22 / 255.0f,
  };
  constexpr ColorRgba neutral{
      .red = 0x6e / 255.0f,
      .green = 0x76 / 255.0f,
      .blue = 0x81 / 255.0f,
  };

  std::array<HierarchyCardState, 3> states;
  states[0].colors = {purple, blue, green, blue, amber, purple, purple};
  states[1].colors = {purple, blue, blue, green, amber, purple, purple};
  states[2].colors = {purple, blue, blue, green, amber, purple, neutral};
  return states;
}

inline void
RecordShellCommandInvocation(ShellGalleryState &state,
                             const steppenface::CommandView &command) {
  state.feedback = "Application command invoked: " + command.label + ".";
}

[[nodiscard]] constexpr steppenface::BackendCapability
GalleryMissingBackendCapability(const steppenface::CommandId command) {
  using steppenface::BackendCapability;
  using steppenface::CommandId;
  switch (command) {
  case CommandId::OpenProject:
  case CommandId::SaveProject:
  case CommandId::SaveProjectAs:
    return BackendCapability::ProjectPersistence;
  case CommandId::ExportFile:
    return BackendCapability::ExportJob;
  case CommandId::OpenSettings:
    return BackendCapability::SettingsPersistence;
  case CommandId::OpenLicense:
    return BackendCapability::LicenseManagement;
  default:
    return BackendCapability::None;
  }
}

[[nodiscard]] inline std::string
GalleryModelBedTarget(const GalleryModelBedToolbarState &bed) {
  return "bed." + std::to_string(bed.id);
}

[[nodiscard]] inline GalleryModelBedToolbarState *
FindGalleryModelBed(GalleryModelToolbarState &state,
                    const std::string_view target) {
  for (GalleryModelBedToolbarState &bed : state.beds) {
    if (GalleryModelBedTarget(bed) == target) {
      return &bed;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const GalleryModelBedToolbarState *
FindGalleryModelBed(const GalleryModelToolbarState &state,
                    const std::string_view target) {
  for (const GalleryModelBedToolbarState &bed : state.beds) {
    if (GalleryModelBedTarget(bed) == target) {
      return &bed;
    }
  }
  return nullptr;
}

[[nodiscard]] inline bool
ApplyGalleryToolbarAction(ShellGalleryState &state,
                          const steppenface::ControlActionView &action) {
  using steppenface::SelectionScope;
  using steppenface::SelectionTool;

  if (action.field.value == "model.selection-tool") {
    if (const SelectionTool *value =
            std::get_if<SelectionTool>(&action.value)) {
      state.model_toolbar.selection_tool = *value;
      state.feedback = "Updated the 3D selection tool.";
      return true;
    }
    return false;
  }
  if (action.field.value == "session.model-grid-target") {
    const std::string *value = std::get_if<std::string>(&action.value);
    if (value == nullptr ||
        (*value != "all" &&
         FindGalleryModelBed(state.model_toolbar, *value) == nullptr)) {
      state.feedback = "Ignored an invalid model grid target.";
      return false;
    }
    state.model_toolbar.grid_target = *value;
    state.feedback = "Updated the model grid target.";
    return true;
  }
  if (action.field.value == "model.grid-spacing") {
    const std::int64_t *value = std::get_if<std::int64_t>(&action.value);
    if (value == nullptr || *value <= 0 || *value > 10'000 ||
        !action.target.has_value()) {
      state.feedback = "Grid spacing must be between 1 and 10000 mm.";
      return false;
    }
    const std::string &target = action.target->value;
    if (target == "all") {
      for (GalleryModelBedToolbarState &bed : state.model_toolbar.beds) {
        bed.grid_spacing_mm = static_cast<int>(*value);
      }
    } else if (GalleryModelBedToolbarState *bed =
                   FindGalleryModelBed(state.model_toolbar, target)) {
      bed->grid_spacing_mm = static_cast<int>(*value);
    } else {
      state.feedback = "Ignored an invalid model grid target.";
      return false;
    }
    state.feedback = "Updated model bed grid settings.";
    return true;
  }
  if (action.field.value == "model.snap") {
    const bool *value = std::get_if<bool>(&action.value);
    if (value == nullptr || !action.target.has_value()) {
      state.feedback = "Ignored model snapping without a bed target.";
      return false;
    }
    const std::string &target = action.target->value;
    if (target == "all") {
      for (GalleryModelBedToolbarState &bed : state.model_toolbar.beds) {
        bed.snap_to_grid = *value;
      }
    } else if (GalleryModelBedToolbarState *bed =
                   FindGalleryModelBed(state.model_toolbar, target)) {
      bed->snap_to_grid = *value;
    } else {
      state.feedback = "Ignored an invalid model grid target.";
      return false;
    }
    state.feedback = "Updated model bed grid settings.";
    return true;
  }
  if (action.field.value == "canvas.selection-scope") {
    if (const SelectionScope *value =
            std::get_if<SelectionScope>(&action.value)) {
      state.canvas_toolbar.selection_scope = *value;
      state.feedback = "Updated the Canvas selection scope.";
      return true;
    }
    return false;
  }
  if (action.field.value == "canvas.selection-tool") {
    if (const SelectionTool *value =
            std::get_if<SelectionTool>(&action.value)) {
      state.canvas_toolbar.selection_tool = *value;
      state.feedback = "Updated the Canvas selection tool.";
      return true;
    }
    return false;
  }
  if (action.field.value == "canvas.grid-visible") {
    if (const bool *value = std::get_if<bool>(&action.value)) {
      state.canvas_toolbar.grid_visible = *value;
      state.feedback =
          *value ? "Showed the Canvas grid." : "Hid the Canvas grid.";
      return true;
    }
    return false;
  }
  if (action.field.value == "canvas.grid-spacing") {
    double value = 0.0;
    if (const double *decimal = std::get_if<double>(&action.value)) {
      value = *decimal;
    } else if (const std::int64_t *integer =
                   std::get_if<std::int64_t>(&action.value)) {
      value = static_cast<double>(*integer);
    } else {
      return false;
    }
    if (!std::isfinite(value) || value <= 0.0) {
      state.feedback = "Grid spacing must be greater than zero.";
      return false;
    }
    state.canvas_toolbar.grid_spacing_mm = value;
    state.feedback = "Updated the Canvas grid spacing.";
    return true;
  }
  constexpr std::string_view snap_prefix = "canvas.snap.";
  if (std::string_view(action.field.value).starts_with(snap_prefix)) {
    const bool *value = std::get_if<bool>(&action.value);
    if (value == nullptr) {
      return false;
    }
    const std::string_view option =
        std::string_view(action.field.value).substr(snap_prefix.size());
    bool *target = nullptr;
    if (option == "guides") {
      target = &state.canvas_toolbar.snap_guides;
    } else if (option == "major-grid") {
      target = &state.canvas_toolbar.snap_major_grid;
    } else if (option == "minor-grid") {
      target = &state.canvas_toolbar.snap_minor_grid;
    } else if (option == "margins") {
      target = &state.canvas_toolbar.snap_margins;
    }
    if (target == nullptr) {
      return false;
    }
    *target = *value;
    state.feedback = "Updated Canvas snapping.";
    return true;
  }
  return false;
}

inline void MergeGalleryShellResult(ShellGalleryState &state,
                                    shell::ApplicationShellState result,
                                    const bool explorer_visible,
                                    const bool inspector_visible) {
  result.explorer_visible = explorer_visible;
  result.inspector_visible = inspector_visible;
  result.operation_tray_visible = state.operation.expanded;
  result.operation_tray_height = state.operation.tray_height;
  state.layout = result;
}

struct GalleryState {
  GalleryTab active_tab = GalleryTab::Components;
  GalleryTab shell_return_tab = GalleryTab::Components;
  std::size_t panel_audit_index = 0;
  std::string panel_audit_query;
  bool focus_active_tab = false;
  ResolvedTheme theme = ResolvedTheme::Dark;
  float scale = 1.0f;
  double spacing = 8.0;
  std::size_t rotation_option = 0;
  int hours = 0;
  int minutes = 5;
  float explode = 38.0f;
  int rotations = 4;
  steppenface::WorkspaceKind component_workspace =
      steppenface::WorkspaceKind::Model3d;
  steppenface::SelectionScope component_selection_scope =
      steppenface::SelectionScope::Canvas;
  steppenface::SelectionTool component_selection_tool =
      steppenface::SelectionTool::Pointer;
  bool availability_selected = true;
  ToggleState checkbox = ToggleState::On;
  ToggleState checkbox_off = ToggleState::Off;
  ToggleState checkbox_mixed = ToggleState::Mixed;
  ToggleState visible = ToggleState::On;
  ToggleState guides_visible = ToggleState::Off;
  ToggleState selection_visible = ToggleState::Mixed;
  ToggleState grain_enabled = ToggleState::On;
  ToggleState grain_disabled = ToggleState::Off;
  ToggleState direction_locked = ToggleState::On;
  ToggleState direction_unlocked = ToggleState::Off;
  std::size_t segmented_index = 0;
  std::size_t tab_index = 0;
  std::string explorer_query;
  ContextMenuState context_menu;
  StatusZoomPopoverState zoom_popover;
  float zoom_percent = 100.0f;
  std::array<std::string, 3> renamable_labels{"Draft", "Production", "Archive"};
  std::size_t renamable_index = 0;
  RenamableSelectState renamable_select;
  std::array<ToggleState, 3> multiselect_states{
      ToggleState::On, ToggleState::Off, ToggleState::Mixed};
  std::size_t radio_index = 0;
  GrainDirectionGizmoState grain_gizmo;
  double grain_degrees = 35.0;
  bool operation_expanded = true;
  float resize_value = 200.0f;
  ModelessWindowState modeless_window;
  ComponentCaptureVariant component_capture = ComponentCaptureVariant::Default;

  std::array<HierarchyCardState, 3> hierarchy_cards =
      DefaultHierarchyCardStates();

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
  bool invalid_issues_expanded = true;
  bool repairable_expanded = true;
  bool warning_issues_expanded = true;
  std::array<ToggleState, 1> invalid_issue_visibility{ToggleState::On};
  std::array<ToggleState, 2> issue_visibility{ToggleState::On,
                                              ToggleState::Off};
  std::array<ToggleState, 1> warning_issue_visibility{ToggleState::On};

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

inline void ActivateGalleryTab(GalleryState &state, const GalleryTab tab) {
  if (IsFullCanvasPreview(tab) && !IsFullCanvasPreview(state.active_tab)) {
    state.shell_return_tab = state.active_tab;
  }
  state.active_tab = tab;
  state.focus_active_tab = false;
}

inline void LeaveShellPreview(GalleryState &state) {
  state.active_tab = IsFullCanvasPreview(state.shell_return_tab)
                         ? GalleryTab::Components
                         : state.shell_return_tab;
  state.focus_active_tab = true;
}

[[nodiscard]] inline bool SeedGalleryCaptureState(GalleryState &state,
                                                  const std::string_view slug) {
  const auto settings = [&state](const SettingsSection section) {
    ActivateGalleryTab(state, GalleryTab::Settings);
    state.settings.active_section = section;
    state.settings.window_open = true;
    state.settings.request_window_focus = true;
  };
  if (slug == "settings-general") {
    settings(SettingsSection::General);
  } else if (slug == "settings-appearance") {
    settings(SettingsSection::Appearance);
  } else if (slug == "settings-machines-profiles") {
    settings(SettingsSection::Machines);
  } else if (slug == "settings-machines-bed-area") {
    settings(SettingsSection::Machines);
    state.settings.active_machine_tab = MachineSettingsTab::BedArea;
  } else if (slug == "settings-machines-information") {
    settings(SettingsSection::Machines);
    static_cast<void>(BeginEditMachine(state.settings, "router-4x8"));
  } else if (slug == "settings-machines-new") {
    settings(SettingsSection::Machines);
    BeginNewMachine(state.settings);
  } else if (slug == "settings-machines-preset-browser") {
    settings(SettingsSection::Machines);
    BeginNewMachine(state.settings);
    static_cast<void>(OpenMachinePresetPicker(state.settings));
  } else if (slug == "settings-machines-origin-confirmation") {
    settings(SettingsSection::Machines);
    BeginNewMachine(state.settings);
    static_cast<void>(OpenMachinePresetPicker(state.settings));
    static_cast<void>(
        ApplyMachinePreset(state.settings, "creality-falcon-cr-5w-10w"));
    state.settings.request_machine_confirmation_scroll = true;
  } else if (slug == "settings-license") {
    settings(SettingsSection::License);
  } else if (slug == "settings-legal") {
    settings(SettingsSection::Legal);
  } else if (slug == "settings-remove-dialog") {
    settings(SettingsSection::Machines);
    state.settings.draft.machines.selected_id = "laser-900";
    state.settings.remove_confirmation_open = true;
  } else if (slug == "settings-discard-dialog") {
    settings(SettingsSection::General);
    state.settings.draft.general.diagnostics_enabled = true;
    RefreshSettingsDerivedState(state.settings);
    state.settings.discard_confirmation_open = true;
  } else if (slug == "hierarchy-actions") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.hierarchy_cards[0].action_row = 1;
    state.hierarchy_cards[0].request_actions = true;
  } else if (slug == "components-context-menu") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::ContextMenuOpen;
  } else if (slug == "components-status-zoom") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::StatusZoomOpen;
  } else if (slug == "components-renaming") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::Renaming;
    state.renamable_select.renaming = true;
    state.renamable_select.original = "Draft";
    state.renamable_select.draft = "Draft";
  } else if (slug == "components-multiselect") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::MultiselectOpen;
  } else if (slug == "components-modeless-window") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::ModelessWindowOpen;
  } else if (slug == "components-grain-bed") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::GrainBed;
    state.grain_degrees = 90.0;
    state.grain_gizmo = {};
  } else if (slug == "components-grain-part") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::GrainPart;
    state.grain_degrees = 0.0;
    state.grain_gizmo = {};
  } else if (slug == "components-grain-paired") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::GrainPaired;
    state.grain_degrees = 35.0;
    state.grain_gizmo = {};
  } else if (slug == "components-grain-dragging") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::GrainDragging;
    state.grain_degrees = 35.0;
    state.grain_gizmo = {};
    state.grain_gizmo.editing = true;
    state.grain_gizmo.pointer_captured = true;
    state.grain_gizmo.original_degrees = 35.0;
    state.grain_gizmo.draft_degrees = 45.0;
  } else if (slug == "components-grain-locked") {
    ActivateGalleryTab(state, GalleryTab::Components);
    state.component_capture = ComponentCaptureVariant::GrainLocked;
    state.grain_degrees = 35.0;
    state.grain_gizmo = {};
  } else if (slug == "shell-canvas-toolbar") {
    ActivateGalleryTab(state, GalleryTab::Shell);
    state.shell.active_workspace = steppenface::WorkspaceKind::Canvas;
  } else if (slug == "shell-model-toolbar") {
    ActivateGalleryTab(state, GalleryTab::Shell);
    state.shell.active_workspace = steppenface::WorkspaceKind::Model3d;
  } else {
    return false;
  }
  return true;
}

void DrawComponentGallery(detail::UiAssetAtlas &assets, GalleryState &state);
[[nodiscard]] bool DrawApplicationShellGallery(detail::UiAssetAtlas &assets,
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
void DrawShellOperationStrip(detail::UiAssetAtlas &assets, GalleryState &state);
void DrawShellStatusBar(detail::UiAssetAtlas &assets, GalleryState &state);

} // namespace fancy_ui::gallery
