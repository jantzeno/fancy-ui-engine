#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/application_chrome.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"
#include "native_panel_contracts.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fancy_ui::gallery {

namespace {

using namespace steppenface;
using UiAvailability = steppenface::Availability;

UiAvailability Enabled() { return {}; }

UiAvailability
Disabled(std::string reason,
         const BackendCapability capability = BackendCapability::None) {
  return {
      .enabled = false,
      .disabled_reason = std::move(reason),
      .missing_capability = capability,
  };
}

CommandView
GalleryCommand(const CommandId command, std::string id, std::string label,
               std::string shortcut = {},
               UiAvailability availability = Enabled(),
               const CommandVariant variant = CommandVariant::Normal) {
  return {
      .id = {.value = std::move(id)},
      .command = command,
      .label = std::move(label),
      .shortcut = std::move(shortcut),
      .variant = variant,
      .availability = std::move(availability),
  };
}

MenuItemView MenuCommand(CommandView command) {
  const UiId id = command.id;
  const std::string label = command.label;
  return {
      .id = id,
      .kind = MenuItemKind::Command,
      .label = label,
      .command = std::move(command),
  };
}

MenuItemView MenuSeparator(std::string id) {
  return {
      .id = {.value = std::move(id)},
      .kind = MenuItemKind::Separator,
  };
}

MenuItemView MenuSubmenu(std::string id, std::string label,
                         std::vector<MenuItemView> children) {
  return {
      .id = {.value = std::move(id)},
      .kind = MenuItemKind::Submenu,
      .label = std::move(label),
      .children = std::move(children),
  };
}

MenuItemView MenuWorkspace(std::string id, std::string label,
                           const WorkspaceKind workspace) {
  return {
      .id = {.value = std::move(id)},
      .kind = MenuItemKind::Workspace,
      .label = std::move(label),
      .workspace = workspace,
  };
}

ApplicationMenuView Menu(std::string id, std::string label,
                         std::vector<MenuItemView> items) {
  return {
      .id = {.value = std::move(id)},
      .label = std::move(label),
      .items = std::move(items),
  };
}

ApplicationBarView BuildGalleryApplicationBar(const ShellGalleryState &state) {
  const WorkspaceKind workspace = state.active_workspace;
  const bool canvas = workspace == WorkspaceKind::Canvas;
  const UiAvailability needs_selection =
      state.has_selection ? Enabled()
                          : Disabled("Select an object or model item first.");
  const UiAvailability canvas_selection =
      !canvas ? Disabled("Available in the Canvas workspace.")
              : (!state.has_selection
                     ? Disabled("Select one or more Canvas objects first.")
                     : Enabled());
  const UiAvailability canvas_only =
      canvas ? Enabled() : Disabled("Available in the Canvas workspace.");
  const UiAvailability assisted_selection =
      !canvas && state.has_model
          ? Enabled()
          : Disabled("Available in the 3D Model activity with a loaded model.");
  const UiAvailability clear_assignment =
      canvas ? (state.has_assigned_selection
                    ? Enabled()
                    : Disabled("Select assigned Canvas artwork first."))
             : (state.has_selection
                    ? Enabled()
                    : Disabled("Select an assigned model part first."));
  const UiAvailability convert =
      !canvas ? Disabled("Available in the Canvas workspace.")
              : (state.can_convert_to_partbed
                     ? Enabled()
                     : Disabled("Select one convertible Canvas object first."));
  ApplicationBarView bar{
      .active_workspace = workspace,
      .document_dirty = true,
      .dirty_label = "Unsaved",
  };
  bar.menus = {
      Menu("menu.file", "File",
           {
               MenuCommand(GalleryCommand(
                   CommandId::OpenProject, "file.open-project", "Open Project…",
                   "Ctrl+O",
                   Disabled("Project persistence is not implemented.",
                            GalleryMissingBackendCapability(
                                CommandId::OpenProject)))),
               MenuCommand(GalleryCommand(
                   CommandId::SaveProject, "file.save-project", "Save Project",
                   "Ctrl+S",
                   Disabled("Project persistence is not implemented.",
                            GalleryMissingBackendCapability(
                                CommandId::SaveProject)))),
               MenuCommand(GalleryCommand(
                   CommandId::SaveProjectAs, "file.save-project-as",
                   "Save Project As…", "Ctrl+Shift+S",
                   Disabled("Project persistence is not implemented.",
                            GalleryMissingBackendCapability(
                                CommandId::SaveProjectAs)))),
               MenuSeparator("file.separator.transfer"),
               MenuCommand(GalleryCommand(CommandId::ImportFiles,
                                          "file.import-files", "Import Files…",
                                          "Ctrl+I")),
               MenuCommand(GalleryCommand(
                   CommandId::ExportFile, "file.export", "Export…", {},
                   Disabled("Export jobs are not implemented.",
                            GalleryMissingBackendCapability(
                                CommandId::ExportFile)))),
               MenuSeparator("file.separator.quit"),
               MenuCommand(GalleryCommand(CommandId::Quit, "file.quit", "Quit",
                                          "Ctrl+Q")),
           }),
      Menu("menu.edit", "Edit",
           {
               MenuCommand(GalleryCommand(CommandId::Undo, "edit.undo", "Undo",
                                          "Ctrl+Z")),
               MenuCommand(GalleryCommand(CommandId::Redo, "edit.redo", "Redo",
                                          "Ctrl+Shift+Z")),
               MenuSeparator("edit.separator.clipboard"),
               MenuCommand(GalleryCommand(CommandId::Cut, "edit.cut", "Cut",
                                          "Ctrl+X", needs_selection)),
               MenuCommand(GalleryCommand(CommandId::Copy, "edit.copy", "Copy",
                                          "Ctrl+C", needs_selection)),
               MenuCommand(GalleryCommand(CommandId::Paste, "edit.paste",
                                          "Paste", "Ctrl+V")),
               MenuSeparator("edit.separator.selection"),
               MenuCommand(GalleryCommand(CommandId::SelectAll,
                                          "edit.select-all", "Select all",
                                          "Ctrl+A")),
               MenuCommand(GalleryCommand(
                   CommandId::ClearSelection, "edit.clear-selection",
                   "Clear selection", "Esc", needs_selection)),
               MenuCommand(GalleryCommand(
                   CommandId::SelectExternalFaces, "edit.select-external",
                   "Select external faces", {}, assisted_selection)),
               MenuCommand(GalleryCommand(
                   CommandId::SelectInternalFaces, "edit.select-internal",
                   "Select internal faces", {}, assisted_selection)),
           }),
      Menu("menu.view", "View",
           {
               MenuWorkspace("view.workspace.3d", "3D workspace",
                             WorkspaceKind::Model3d),
               MenuWorkspace("view.workspace.canvas", "Canvas workspace",
                             WorkspaceKind::Canvas),
               MenuSeparator("view.separator.visibility"),
               MenuCommand(GalleryCommand(CommandId::HideAll, "view.hide-all",
                                          "Hide all")),
               MenuCommand(GalleryCommand(CommandId::ShowAll, "view.show-all",
                                          "Show all", "Shift+H")),
               MenuCommand(GalleryCommand(CommandId::HideSelected,
                                          "view.hide-selected", "Hide selected",
                                          "H", needs_selection)),
               MenuCommand(GalleryCommand(CommandId::IsolateSelected,
                                          "view.isolate", "Isolate selected",
                                          "I", needs_selection)),
               MenuSeparator("view.separator.zoom"),
               MenuCommand(GalleryCommand(CommandId::ZoomToFit, "view.zoom-fit",
                                          "Zoom to fit", "F")),
               MenuCommand(GalleryCommand(
                   CommandId::ZoomToSelection, "view.zoom-selection",
                   "Zoom to selection", "Shift+F", needs_selection)),
               MenuCommand(GalleryCommand(
                   CommandId::ZoomActualSize, "view.zoom-actual", "Actual size",
                   "1",
                   workspace == WorkspaceKind::Canvas
                       ? Enabled()
                       : Disabled("Actual size is available in Canvas."))),
           }),
      Menu("menu.object", "Object",
           {
               MenuCommand(GalleryCommand(CommandId::GroupSelection,
                                          "object.group", "Group", "Ctrl+G",
                                          canvas_selection)),
               MenuCommand(GalleryCommand(CommandId::UngroupSelection,
                                          "object.ungroup", "Ungroup",
                                          "Ctrl+Shift+G", canvas_selection)),
               MenuCommand(GalleryCommand(CommandId::ExtractSelection,
                                          "object.extract", "Extract", {},
                                          canvas_selection)),
               MenuSeparator("object.separator.transform"),
               MenuSubmenu(
                   "object.transform", "Transform",
                   {
                       MenuCommand(GalleryCommand(
                           CommandId::FlipHorizontal, "object.flip-horizontal",
                           "Flip horizontal", {}, canvas_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::FlipVertical, "object.flip-vertical",
                           "Flip vertical", {}, canvas_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::RotateCW, "object.rotate-clockwise",
                           "Rotate clockwise", {}, canvas_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::RotateCCW,
                           "object.rotate-counterclockwise",
                           "Rotate counterclockwise", {}, canvas_selection)),
                   }),
               MenuSubmenu("object.assignment", "Assignment",
                           {
                               MenuCommand(GalleryCommand(
                                   CommandId::ConvertObjectToPartBed,
                                   "object.convert-to-bed",
                                   "Convert to part bed", {}, convert)),
                               MenuCommand(GalleryCommand(
                                   CommandId::ClearAssignment,
                                   "object.clear-assignment",
                                   "Clear assignment", {}, clear_assignment)),
                           }),
               MenuSeparator("object.separator.delete"),
               MenuCommand(GalleryCommand(
                   CommandId::DeleteObjects, "object.delete", "Delete",
                   "Delete", canvas_selection, CommandVariant::Destructive)),
           }),
      Menu(
          "menu.tools", "Tools",
          {
              MenuCommand(GalleryCommand(CommandId::AnalyzeContours,
                                         "tools.analyze", "Analyze contours",
                                         {}, canvas_selection)),
              MenuSubmenu(
                  "tools.select-issues", "Select issues",
                  {
                      MenuCommand(GalleryCommand(
                          CommandId::SelectDegenerateObjects,
                          "tools.select-degenerate", "Degenerate objects", {},
                          canvas_only)),
                      MenuCommand(
                          GalleryCommand(CommandId::SelectInvalidObjects,
                                         "tools.select-invalid",
                                         "Invalid objects", {}, canvas_only)),
                      MenuCommand(GalleryCommand(
                          CommandId::SelectOpenObjects, "tools.select-open",
                          "Open objects", {}, canvas_only)),
                      MenuCommand(GalleryCommand(
                          CommandId::SelectSelfIntersectingObjects,
                          "tools.select-self-intersecting",
                          "Self-intersections", {}, canvas_only)),
                  }),
              MenuCommand(GalleryCommand(
                  CommandId::ConvertObjectToPartBed, "tools.convert-to-bed",
                  "Convert object to part bed", {}, convert)),
              MenuSubmenu(
                  "tools.deconstruction", "Deconstruction",
                  {
                      MenuCommand(GalleryCommand(
                          CommandId::ClearPreview, "tools.clear-preview",
                          "Clear preview", {}, canvas_only)),
                      MenuCommand(GalleryCommand(
                          CommandId::PreviewGuideSplit, "tools.preview-guide",
                          "Preview guide split", {}, canvas_selection)),
                      MenuCommand(GalleryCommand(
                          CommandId::ApplyGuideSplit, "tools.apply-guide",
                          "Apply guide split", {}, canvas_selection)),
                      MenuCommand(
                          GalleryCommand(CommandId::PreviewAutoSplitHorizontal,
                                         "tools.preview-auto-horizontal",
                                         "Preview horizontal auto-split", {},
                                         canvas_selection)),
                      MenuCommand(GalleryCommand(
                          CommandId::PreviewAutoSplitVertical,
                          "tools.preview-auto-vertical",
                          "Preview vertical auto-split", {}, canvas_selection)),
                      MenuCommand(GalleryCommand(
                          CommandId::PreviewAutoSplitBoth,
                          "tools.preview-auto-both", "Preview both directions",
                          {}, canvas_selection)),
                      MenuCommand(GalleryCommand(
                          CommandId::ApplyAutoSplit, "tools.apply-auto",
                          "Apply auto-split", {}, canvas_selection)),
                  }),
              MenuSubmenu(
                  "tools.repair", "Repair",
                  {
                      MenuCommand(GalleryCommand(
                          CommandId::JoinOpenSegments, "tools.join-open",
                          "Join open segments", {}, canvas_selection)),
                      MenuCommand(GalleryCommand(
                          CommandId::RepairOrphanHoles, "tools.repair-orphan",
                          "Repair orphan holes", {}, canvas_selection)),
                      MenuCommand(GalleryCommand(
                          CommandId::ReindexObjects, "tools.reindex",
                          "Reindex objects", {}, canvas_only)),
                  }),
              MenuSeparator("tools.separator.settings"),
              MenuCommand(GalleryCommand(
                  CommandId::OpenSettings, "tools.settings", "Settings…",
                  "Ctrl+,",
                  Disabled("Settings persistence is not implemented.",
                           GalleryMissingBackendCapability(
                               CommandId::OpenSettings)))),
          }),
      Menu("menu.help", "Help",
           {
               MenuCommand(GalleryCommand(
                   CommandId::OpenLicense, "help.license", "License…", {},
                   Disabled("License management is not implemented.",
                            GalleryMissingBackendCapability(
                                CommandId::OpenLicense)))),
               MenuCommand(GalleryCommand(CommandId::OpenLegalNotices,
                                          "help.legal", "Legal notices…")),
           }),
  };
  return bar;
}

ControlActionView GalleryAction(std::string field, FieldValue value,
                                std::string target = {}) {
  return {
      .field = {.value = std::move(field)},
      .value = std::move(value),
      .target = target.empty()
                    ? std::optional<UiId>{}
                    : std::optional<UiId>{UiId{.value = std::move(target)}},
      .availability = Enabled(),
  };
}

FieldView GalleryNumericField(std::string id, std::string label,
                              const double value, std::string unit,
                              std::string help, const bool integral,
                              std::optional<UiId> target = std::nullopt) {
  const UiId field{.value = std::move(id)};
  return {
      .id = field,
      .label = std::move(label),
      .help = std::move(help),
      .edit = EditBindingView{.field = field, .target = std::move(target)},
      .content =
          NumericFieldView{
              .value = value,
              .unit = std::move(unit),
              .integral = integral,
          },
  };
}

std::string GalleryModelGridLabel(const GalleryModelToolbarState &state) {
  const int spacing = state.beds.front().grid_spacing_mm;
  const bool uniform =
      std::all_of(state.beds.begin(), state.beds.end(),
                  [spacing](const GalleryModelBedToolbarState &bed) {
                    return bed.grid_spacing_mm == spacing;
                  });
  return uniform ? std::format("{} mm", spacing) : "Mixed";
}

std::string GalleryModelSnapLabel(const GalleryModelToolbarState &state) {
  const bool first = state.beds.front().snap_to_grid;
  const bool uniform =
      std::all_of(state.beds.begin(), state.beds.end(),
                  [first](const GalleryModelBedToolbarState &bed) {
                    return bed.snap_to_grid == first;
                  });
  return uniform ? (first ? "On" : "Off") : "Mixed";
}

int GalleryModelGridSpacing(const GalleryModelToolbarState &state) {
  if (state.grid_target == "all") {
    return state.beds.front().grid_spacing_mm;
  }
  const GalleryModelBedToolbarState *bed =
      FindGalleryModelBed(state, state.grid_target);
  return bed == nullptr ? state.beds.front().grid_spacing_mm
                        : bed->grid_spacing_mm;
}

ToolbarPopoverView
BuildGalleryModelGridPopover(const GalleryModelToolbarState &state) {
  const std::string target =
      state.grid_target == "all" ||
              FindGalleryModelBed(state, state.grid_target) != nullptr
          ? state.grid_target
          : "all";
  ToolbarPopoverView popover{
      .id = {.value = "model.grid"},
      .label = "Grid: " + GalleryModelGridLabel(state),
      .tooltip = "Set exact grid spacing for all beds or a specific bed",
      .availability = Enabled(),
  };
  popover.items.push_back(ToolbarMenuItemView{
      .id = {.value = "model.grid-target.all"},
      .label = "All beds",
      .secondary_label = GalleryModelGridLabel(state),
      .selected = target == "all",
      .action = GalleryAction("session.model-grid-target", std::string{"all"}),
  });
  for (const GalleryModelBedToolbarState &bed : state.beds) {
    const std::string bed_target = GalleryModelBedTarget(bed);
    popover.items.push_back(ToolbarMenuItemView{
        .id = {.value = "model.grid-target." + std::to_string(bed.id)},
        .label = bed.name,
        .secondary_label = std::format("{} mm", bed.grid_spacing_mm),
        .selected = target == bed_target,
        .action = GalleryAction("session.model-grid-target", bed_target),
    });
  }
  const int current_spacing = GalleryModelGridSpacing(state);
  for (const int spacing : {5, 10, 25, 50}) {
    popover.items.push_back(ToolbarMenuItemView{
        .id = {.value = "model.grid-spacing." + std::to_string(spacing)},
        .label = std::format("{} mm", spacing),
        .selected = current_spacing == spacing,
        .separator_before = spacing == 5,
        .action = GalleryAction("model.grid-spacing",
                                static_cast<std::int64_t>(spacing), target),
    });
  }
  popover.fields.push_back(GalleryNumericField(
      "model.grid-spacing", "Custom spacing", current_spacing, "mm",
      "Enter a positive whole-millimeter spacing.", true,
      UiId{.value = target}));
  return popover;
}

ToolbarPopoverView
BuildGalleryModelSnapPopover(const GalleryModelToolbarState &state) {
  ToolbarPopoverView popover{
      .id = {.value = "model.snap"},
      .label = "Snap: " + GalleryModelSnapLabel(state),
      .tooltip = "Set grid snap for all beds or a specific bed",
      .availability = Enabled(),
  };
  const bool all_enabled = std::all_of(
      state.beds.begin(), state.beds.end(),
      [](const GalleryModelBedToolbarState &bed) { return bed.snap_to_grid; });
  popover.items.push_back(ToolbarMenuItemView{
      .id = {.value = "model.snap.all"},
      .label = "All beds",
      .secondary_label = all_enabled ? "On" : "Off",
      .selected = all_enabled,
      .action = GalleryAction("model.snap", !all_enabled, "all"),
  });
  for (const GalleryModelBedToolbarState &bed : state.beds) {
    popover.items.push_back(ToolbarMenuItemView{
        .id = {.value = "model.snap." + std::to_string(bed.id)},
        .label = bed.name,
        .secondary_label = bed.snap_to_grid ? "On" : "Off",
        .selected = bed.snap_to_grid,
        .separator_before = &bed == &state.beds.front(),
        .action = GalleryAction("model.snap", !bed.snap_to_grid,
                                GalleryModelBedTarget(bed)),
    });
  }
  return popover;
}

ToolbarPopoverView
BuildGalleryCanvasGridPopover(const GalleryCanvasToolbarState &state) {
  ToolbarPopoverView popover{
      .id = {.value = "canvas.grid"},
      .label = std::format("Grid: {:.0f} mm", state.grid_spacing_mm),
      .tooltip = "Configure the Canvas grid",
      .availability = Enabled(),
  };
  popover.items.push_back(ToolbarMenuItemView{
      .id = {.value = "canvas.grid.visible"},
      .label = "Show grid",
      .secondary_label = state.grid_visible ? "On" : "Off",
      .selected = state.grid_visible,
      .action = GalleryAction("canvas.grid-visible", !state.grid_visible),
  });
  for (const int spacing : {5, 10, 25, 50}) {
    popover.items.push_back(ToolbarMenuItemView{
        .id = {.value = "canvas.grid.spacing." + std::to_string(spacing)},
        .label = std::format("{} mm", spacing),
        .selected = std::abs(state.grid_spacing_mm -
                             static_cast<double>(spacing)) < 0.01,
        .separator_before = spacing == 5,
        .action = GalleryAction("canvas.grid-spacing",
                                static_cast<std::int64_t>(spacing)),
    });
  }
  popover.fields.push_back(GalleryNumericField(
      "canvas.grid-spacing", "Custom spacing", state.grid_spacing_mm, "mm",
      "Enter a positive grid spacing.", false));
  return popover;
}

ToolbarPopoverView
BuildGalleryCanvasSnapPopover(const GalleryCanvasToolbarState &state) {
  const bool snap_on = state.snap_guides || state.snap_major_grid ||
                       state.snap_minor_grid || state.snap_margins;
  ToolbarPopoverView popover{
      .id = {.value = "canvas.snap"},
      .label = std::string{"Snap: "} + (snap_on ? "On" : "Off"),
      .tooltip = "Configure Canvas snapping",
      .availability = Enabled(),
  };
  const auto add_item = [&popover](const std::string_view id,
                                   const std::string_view label,
                                   const bool selected) {
    popover.items.push_back(ToolbarMenuItemView{
        .id = {.value = "canvas.snap." + std::string{id}},
        .label = std::string{label},
        .secondary_label = selected ? "On" : "Off",
        .selected = selected,
        .action = GalleryAction("canvas.snap." + std::string{id}, !selected),
    });
  };
  add_item("guides", "Guides", state.snap_guides);
  add_item("major-grid", "Major grid", state.snap_major_grid);
  add_item("minor-grid", "Minor grid", state.snap_minor_grid);
  add_item("margins", "Margins", state.snap_margins);
  return popover;
}

ContextToolbarView BuildGalleryContextToolbar(const ShellGalleryState &state) {
  const WorkspaceKind workspace = state.active_workspace;
  ContextToolbarView toolbar;
  if (workspace == WorkspaceKind::Model3d) {
    const UiAvailability assisted_selection =
        state.has_model
            ? Enabled()
            : Disabled("Available in the Model activity when a model is "
                       "loaded.");
    const UiAvailability clear_selection =
        state.has_selection ? Enabled()
                            : Disabled("No model selection to clear.");
    toolbar.items.emplace_back(ToolbarSegmentedView{
        .id = {.value = "model.selection-tool"},
        .choices =
            {
                {.id = {.value = "model.selection.pointer"},
                 .label = "Pointer",
                 .tooltip = "Select model faces with the pointer",
                 .selected = state.model_toolbar.selection_tool ==
                             SelectionTool::Pointer,
                 .action = GalleryAction("model.selection-tool",
                                         SelectionTool::Pointer)},
                {.id = {.value = "model.selection.rectangle"},
                 .label = "Rectangle",
                 .tooltip = "Select one face for each enclosed visible part",
                 .selected = state.model_toolbar.selection_tool ==
                             SelectionTool::Rectangle,
                 .action = GalleryAction("model.selection-tool",
                                         SelectionTool::Rectangle)},
            },
    });
    toolbar.items.emplace_back(
        ToolbarSeparatorView{.id = {.value = "model.selection-separator"}});
    ToolbarPopoverView select_faces{
        .id = {.value = "model.select-faces"},
        .label = "Select faces",
        .tooltip = "Choose external or internal faces for the active model",
        .availability = assisted_selection,
    };
    CommandView external_faces = GalleryCommand(
        CommandId::SelectExternalFaces, "model.select-faces.external",
        "External Faces", {}, assisted_selection);
    external_faces.tooltip = "Select outward-facing faces for the active model";
    select_faces.items.emplace_back(std::move(external_faces));
    CommandView internal_faces = GalleryCommand(
        CommandId::SelectInternalFaces, "model.select-faces.internal",
        "Internal Faces", {}, assisted_selection);
    internal_faces.tooltip =
        "Select assembly-facing faces for the active model";
    select_faces.items.emplace_back(std::move(internal_faces));
    toolbar.items.emplace_back(std::move(select_faces));
    toolbar.items.emplace_back(GalleryCommand(
        CommandId::ClearSelection, "model.clear-selection", "Clear selection",
        {}, clear_selection, CommandVariant::Tertiary));
    toolbar.items.emplace_back(
        ToolbarSpacerView{.id = {.value = "model.toolbar-spacer"}});
    toolbar.items.emplace_back(
        BuildGalleryModelGridPopover(state.model_toolbar));
    toolbar.items.emplace_back(
        BuildGalleryModelSnapPopover(state.model_toolbar));
    return toolbar;
  }

  toolbar.items.emplace_back(ToolbarSegmentedView{
      .id = {.value = "canvas.selection-scope"},
      .choices =
          {
              {.id = {.value = "canvas.scope.canvas"},
               .label = "Canvas",
               .tooltip = "Select Canvas-level content",
               .selected = state.canvas_toolbar.selection_scope ==
                           SelectionScope::Canvas,
               .action = GalleryAction("canvas.selection-scope",
                                       SelectionScope::Canvas)},
              {.id = {.value = "canvas.scope.object"},
               .label = "Object",
               .tooltip = "Select object content",
               .selected = state.canvas_toolbar.selection_scope ==
                           SelectionScope::Object,
               .action = GalleryAction("canvas.selection-scope",
                                       SelectionScope::Object)},
          },
  });
  toolbar.items.emplace_back(
      ToolbarSeparatorView{.id = {.value = "canvas.scope-separator"}});
  toolbar.items.emplace_back(ToolbarSegmentedView{
      .id = {.value = "canvas.selection-tool"},
      .choices =
          {
              {.id = {.value = "canvas.tool.pointer"},
               .label = "Pointer",
               .tooltip = "Select with the pointer",
               .selected = state.canvas_toolbar.selection_tool ==
                           SelectionTool::Pointer,
               .action = GalleryAction("canvas.selection-tool",
                                       SelectionTool::Pointer)},
              {.id = {.value = "canvas.tool.rectangle"},
               .label = "Rectangle",
               .tooltip = "Select within a rectangle",
               .selected = state.canvas_toolbar.selection_tool ==
                           SelectionTool::Rectangle,
               .action = GalleryAction("canvas.selection-tool",
                                       SelectionTool::Rectangle)},
              {.id = {.value = "canvas.tool.oval"},
               .label = "Oval",
               .tooltip = "Select within an oval",
               .selected =
                   state.canvas_toolbar.selection_tool == SelectionTool::Oval,
               .action =
                   GalleryAction("canvas.selection-tool", SelectionTool::Oval)},
          },
  });
  toolbar.items.emplace_back(
      ToolbarSeparatorView{.id = {.value = "canvas.tool-separator"}});
  toolbar.items.emplace_back(GalleryCommand(
      CommandId::ClearSelection, "canvas.clear-selection", "Clear selection",
      {},
      state.has_selection ? Enabled()
                          : Disabled("No Canvas selection to clear."),
      CommandVariant::Tertiary));
  toolbar.items.emplace_back(
      ToolbarSpacerView{.id = {.value = "canvas.toolbar-spacer"}});
  toolbar.items.emplace_back(
      BuildGalleryCanvasGridPopover(state.canvas_toolbar));
  toolbar.items.emplace_back(
      BuildGalleryCanvasSnapPopover(state.canvas_toolbar));
  return toolbar;
}

FieldView GalleryField(std::string id, std::string label, FieldContent content,
                       UiAvailability availability = Enabled(),
                       std::string help = {}, const bool editable = true) {
  const UiId field{.value = std::move(id)};
  return {
      .id = field,
      .label = std::move(label),
      .help = std::move(help),
      .availability = std::move(availability),
      .edit =
          editable
              ? std::optional<EditBindingView>{EditBindingView{.field = field}}
              : std::nullopt,
      .content = std::move(content),
  };
}

std::vector<ChoiceOptionView> GalleryMaterialOptions() {
  return {
      {.id = {.value = "plywood"}, .label = "Plywood"},
      {.id = {.value = "acrylic"}, .label = "Acrylic"},
      {.id = {.value = "aluminum"}, .label = "Aluminum"},
  };
}

UiId GalleryMaterialId(const std::size_t index) {
  static constexpr std::array ids{"plywood", "acrylic", "aluminum"};
  return {.value = ids[std::min(index, ids.size() - 1)]};
}

std::vector<ActivityView> BuildGalleryActivities(const GalleryState &state) {
  return {
      {.destination = Destination::Model, .label = "Model", .icon = "model"},
      {.destination = Destination::ModelBeds, .label = "Beds", .icon = "bed"},
      {.destination = Destination::CanvasObjects,
       .label = "Objects",
       .icon = "objects"},
      {.destination = Destination::CanvasBeds, .label = "Beds", .icon = "bed"},
      {.destination = Destination::CanvasGrain,
       .label = "Grain",
       .icon = "grain"},
      {.destination = Destination::Search, .label = "Search", .icon = "search"},
      {.destination = Destination::Compact,
       .label = "Compact",
       .icon = "compact"},
      {.destination = Destination::Diagnostics,
       .label = "Diagnostics",
       .icon = "diagnostics",
       .availability = state.settings.applied.general.diagnostics_enabled
                           ? Enabled()
                           : Disabled("Enable diagnostics in Settings.")},
  };
}

ApplicationView BuildGalleryApplicationView(const GalleryState &state) {
  const ShellGalleryState &shell = state.shell;
  ApplicationView view{
      .revision = 1,
      .theme_mode = state.theme == ResolvedTheme::Dark ? ThemeMode::Dark
                                                       : ThemeMode::Light,
      .application_bar = BuildGalleryApplicationBar(shell),
      .context_toolbar = BuildGalleryContextToolbar(shell),
      .panel =
          {
              .id = {.value = "gallery:application-shell"},
              .label = "Application shell preview",
              .destination = shell.active_workspace == WorkspaceKind::Canvas
                                 ? Destination::CanvasObjects
                                 : Destination::Model,
          },
  };
  view.activities = BuildGalleryActivities(state);

  view.panel.explorer = {
      .title = "Objects",
      .tree_label = "Hierarchy",
      .footer = "3 objects · one hidden",
      .search = {.id = {.value = "gallery.explorer.search"},
                 .placeholder = "Search objects, sources, parts"},
      .rows =
          {
              {
                  .id = {.value = "gallery.row.project"},
                  .entity = {.value = "gallery.project"},
                  .label = "fixture-kit-07.svg",
                  .secondary_label = "3 objects",
                  .icon = "objects",
                  .expanded = true,
                  .expandable = true,
              },
              {
                  .id = {.value = "gallery.row.face-plate"},
                  .entity = {.value = "gallery.face-plate"},
                  .label = shell.object_name,
                  .secondary_label = "Plywood",
                  .icon = "objects",
                  .depth = 1,
                  .selected = true,
                  .color = shell.display_color,
                  .color_edit =
                      EditBindingView{
                          .field = {.value = "matrix.display-color"}},
                  .visibility = shell.visible,
                  .visibility_edit =
                      EditBindingView{.field = {.value = "matrix.visibility"}},
                  .context_menu =
                      ContextMenuView{
                          .id = {.value = "gallery.row.face-plate.menu"},
                          .items =
                              {
                                  {.id = {.value = "gallery.row.focus"},
                                   .label = "Focus",
                                   .command = GalleryCommand(
                                       CommandId::ZoomToSelection,
                                       "gallery.row.focus", "Focus")},
                                  {.id = {.value = "gallery.row.visibility"},
                                   .label = "Toggle visibility",
                                   .action = GalleryAction(
                                       "matrix.visibility",
                                       shell.visible == ToggleState::On
                                           ? ToggleState::Off
                                           : ToggleState::On)},
                              }},
              },
              {
                  .id = {.value = "gallery.row.restricted"},
                  .entity = {.value = "gallery.restricted"},
                  .label = "Locked reference",
                  .secondary_label = "Unavailable",
                  .icon = "objects",
                  .depth = 1,
                  .availability = Disabled("Unlock the reference to edit it."),
              },
          },
  };

  view.workspace = {
      .kind = shell.active_workspace,
      .selection_source = {.value =
                               shell.active_workspace == WorkspaceKind::Canvas
                                   ? "workspace.canvas"
                                   : "workspace.model"},
      .title = "Contract matrix workspace",
      .empty_message = "No surface is bound in this gallery composition.",
  };

  SectionView information{
      .id = {.value = "matrix.information"},
      .heading = "Information tree",
      .summary = "Prepared selection",
      .information_rows =
          {
              {.id = {.value = "matrix.info.root"},
               .entity = {.value = "gallery.face-plate"},
               .label = shell.object_name,
               .metadata = "Selected",
               .expanded = true,
               .expandable = true,
               .selected = true,
               .metrics =
                   {{.label = "Width", .value = "240 mm"},
                    {.label = "Height", .value = "180 mm", .stacked = true}},
               .visibility = shell.visible,
               .visibility_edit =
                   EditBindingView{.field = {.value = "matrix.visibility"}},
               .actions = {{.id = {.value = "matrix.info.focus"},
                            .command = CommandId::ZoomToSelection,
                            .label = "…",
                            .tooltip = "Focus selection"}}},
              {.id = {.value = "matrix.info.child"},
               .entity = {.value = "gallery.face-plate.source"},
               .label = "Source geometry",
               .metadata = "fixture-kit-07.svg",
               .depth = 1,
               .highlighted = true,
               .tone = SemanticTone::Information},
          },
  };

  const std::vector<ChoiceOptionView> materials = GalleryMaterialOptions();
  const UiId material = GalleryMaterialId(shell.material_index);
  SectionView fields{
      .id = {.value = "matrix.fields"},
      .heading = "Field contract matrix",
      .summary = "14 payloads",
      .fields =
          {
              GalleryField("matrix.name", "Name",
                           TextFieldView{.value = shell.object_name,
                                         .placeholder = "Object name"}),
              GalleryField("matrix.spacing", "Spacing",
                           NumericFieldView{.value = shell.spacing_mm,
                                            .minimum = 0.0,
                                            .unit = "mm",
                                            .format = "%.1f"}),
              GalleryField(
                  "matrix.material", "Material",
                  SelectFieldView{.options = materials, .selected = material}),
              GalleryField(
                  "matrix.renamable-material", "Named material",
                  RenamableSelectFieldView{
                      .options = materials,
                      .selected = material,
                      .rename = {.field = {.value = "matrix.material-name"}}}),
              GalleryField(
                  "matrix.layers", "Layers",
                  MultiselectFieldView{
                      .summary = shell.enabled == ToggleState::On ? "2 of 2"
                                                                  : "1 of 2",
                      .options =
                          {{{.id = {.value = "geometry"}, .label = "Geometry"},
                            ToggleState::On},
                           {{.id = {.value = "labels"}, .label = "Labels"},
                            shell.enabled}}}),
              GalleryField("matrix.mode", "Mode",
                           SegmentedFieldView{.options = materials,
                                              .selected = material}),
              GalleryField("matrix.enabled", "Enabled",
                           CheckboxFieldView{.state = shell.enabled}),
              GalleryField("matrix.visibility", "Show overlay",
                           VisibilityFieldView{.state = shell.visible}),
              GalleryField("matrix.explode", "Explode",
                           SliderFieldView{.value = shell.explode_percent,
                                           .minimum = 0.0f,
                                           .maximum = 100.0f,
                                           .unit = "%",
                                           .format = "%.0f"}),
              GalleryField(
                  "matrix.rotations", "Search rotations",
                  RotationCompassFieldView{.count = shell.rotation_count}),
              GalleryField(
                  "matrix.duration", "Time limit",
                  DurationFieldView{.value = {.hours = shell.hours,
                                              .minutes = shell.minutes}}),
              GalleryField("matrix.display-color", "Display color",
                           ColorFieldView{.value = shell.display_color}),
              GalleryField(
                  "matrix.source", "Source",
                  ValueFieldView{.value = "fixture-kit-07.svg", .mixed = false},
                  Enabled(), {}, false),
              GalleryField(
                  "matrix.inspect", "",
                  ButtonFieldView{.command = GalleryCommand(
                                      CommandId::OpenLegalNotices,
                                      "matrix.inspect", "Inspect component", {},
                                      Enabled(), CommandVariant::Primary)},
                  Enabled(), {}, false),
              GalleryField("matrix.unavailable", "Unavailable value",
                           NumericFieldView{.value = 12.0},
                           Disabled("Complete setup before editing.")),
              GalleryField("matrix.busy", "Busy value",
                           NumericFieldView{.value = 24.0},
                           UiAvailability{.enabled = false,
                                          .busy = true,
                                          .disabled_reason =
                                              "Calculation in progress."}),
          },
      .status =
          StatusCardView{
              .id = {.value = "matrix.ready"},
              .title = "Ready",
              .message =
                  "Every panel-audit field payload uses the typed renderer.",
              .tone = SemanticTone::Success,
              .icon = "success",
          },
  };
  view.panel.inspector = {
      .title = "Panel audit",
      .subtitle = "Typed ImGui contract matrix",
      .scope = "Selection · 1 object",
      .note = shell.feedback,
      .sections = {std::move(information), std::move(fields)},
      .primary_command = GalleryCommand(CommandId::Quit, "gallery.back",
                                        "Back to component gallery", {},
                                        Enabled(), CommandVariant::Tertiary),
  };
  view.operation = OperationView{
      .id = {.value = "matrix.operation"},
      .title = "Panel audit validation",
      .summary = "Rendering the native contract matrix.",
      .tone = SemanticTone::Success,
      .progress = 1.0f,
  };
  view.status_items = {
      {.id = {.value = "matrix.status.contract"},
       .label = "14 field payloads",
       .tone = SemanticTone::Success},
      {.id = {.value = "matrix.status.theme"},
       .label = state.theme == ResolvedTheme::Dark ? "Dark" : "Light"},
  };
  return view;
}

bool ApplyGalleryPanelEdit(ShellGalleryState &state, const EditField &edit) {
  if (edit.field.value == "matrix.name" ||
      edit.field.value == "matrix.material-name") {
    if (const std::string *value = std::get_if<std::string>(&edit.value)) {
      state.object_name = *value;
      return true;
    }
  } else if (edit.field.value == "matrix.spacing") {
    if (const double *value = std::get_if<double>(&edit.value)) {
      state.spacing_mm = *value;
      return true;
    }
  } else if (edit.field.value == "matrix.material" ||
             edit.field.value == "matrix.renamable-material" ||
             edit.field.value == "matrix.mode") {
    if (const UiId *value = std::get_if<UiId>(&edit.value)) {
      const std::array ids{"plywood", "acrylic", "aluminum"};
      const auto found = std::ranges::find(ids, value->value);
      if (found != ids.end()) {
        state.material_index =
            static_cast<std::size_t>(std::distance(ids.begin(), found));
        return true;
      }
    }
  } else if (edit.field.value == "matrix.layers") {
    if (const ChoiceToggleValue *value =
            std::get_if<ChoiceToggleValue>(&edit.value);
        value != nullptr && value->option.value == "labels") {
      state.enabled = value->state;
      return true;
    }
  } else if (edit.field.value == "matrix.enabled") {
    if (const ToggleState *value = std::get_if<ToggleState>(&edit.value)) {
      state.enabled = *value;
      return true;
    }
  } else if (edit.field.value == "matrix.visibility") {
    if (const ToggleState *value = std::get_if<ToggleState>(&edit.value)) {
      state.visible = *value;
      return true;
    }
  } else if (edit.field.value == "matrix.explode") {
    if (const double *value = std::get_if<double>(&edit.value)) {
      state.explode_percent = static_cast<float>(*value);
      return true;
    }
  } else if (edit.field.value == "matrix.rotations") {
    if (const std::int64_t *value = std::get_if<std::int64_t>(&edit.value)) {
      state.rotation_count = static_cast<int>(*value);
      return true;
    }
  } else if (edit.field.value == "matrix.duration") {
    if (const DurationValue *value = std::get_if<DurationValue>(&edit.value)) {
      state.hours = value->hours;
      state.minutes = value->minutes;
      return true;
    }
  } else if (edit.field.value == "matrix.display-color") {
    if (const ColorRgba *value = std::get_if<ColorRgba>(&edit.value)) {
      state.display_color = *value;
      return true;
    }
  }
  return ApplyGalleryToolbarAction(
      state, {.field = edit.field, .value = edit.value, .target = edit.target});
}

bool ContainsCaseInsensitive(const std::string_view text,
                             const std::string_view query) {
  return std::ranges::search(
             text, query,
             [](const char left, const char right) {
               return std::tolower(static_cast<unsigned char>(left)) ==
                      std::tolower(static_cast<unsigned char>(right));
             })
             .begin() != text.end();
}

void DrawPanelAuditMenu(detail::UiAssetAtlas &assets,
                        const std::vector<PanelContractView> &audits,
                        GalleryState &state, bool &return_requested) {
  if (assets.heading_font() != nullptr) {
    ImGui::PushFont(assets.heading_font(),
                    CurrentLayoutMetrics().typography.page_title_font_height);
  }
  ImGui::TextUnformatted("Panel audits");
  if (assets.heading_font() != nullptr) {
    ImGui::PopFont();
  }
  detail::DrawSecondaryText(std::format(
      "{} canonical Explorer / Inspector contracts", audits.size()));
  ImGui::Spacing();
  if (Button({.id = "panel-audits.back",
              .label = "Back to gallery",
              .variant = ButtonVariant::Tertiary})
          .activated) {
    return_requested = true;
  }

  const ExplorerSearchResult search = ExplorerSearch({
      .id = "panel-audits.search",
      .placeholder = "Filter by audit label or ID",
      .query = state.panel_audit_query,
  });
  if (search.changed) {
    state.panel_audit_query = search.query;
  }
  ImGui::Separator();
  if (ImGui::BeginChild("##panel-audit-list")) {
    for (std::size_t index = 0; index < audits.size(); ++index) {
      const PanelContractView &audit = audits[index];
      if (!state.panel_audit_query.empty() &&
          !ContainsCaseInsensitive(audit.label, state.panel_audit_query) &&
          !ContainsCaseInsensitive(audit.id.value, state.panel_audit_query)) {
        continue;
      }
      ImGui::PushID(audit.id.value.c_str());
      if (Button({.id = "select",
                  .label = audit.label,
                  .selected = index == state.panel_audit_index,
                  .size = {.x = -1.0f, .y = 32.0f}})
              .activated) {
        state.panel_audit_index = index;
      }
      detail::DrawSecondaryText(audit.id.value);
      ImGui::Spacing();
      ImGui::PopID();
    }
  }
  ImGui::EndChild();
}

bool DrawPanelAuditGallery(detail::UiAssetAtlas &assets, GalleryState &state) {
  static ApplicationUi ui(assets);
  static const std::vector<PanelContractView> audits =
      BuildCanonicalPanelAuditContracts();
  if (audits.empty()) {
    return false;
  }
  state.panel_audit_index =
      std::min(state.panel_audit_index, audits.size() - 1);
  ApplicationView view{
      .revision = 1,
      .theme_mode = state.theme == ResolvedTheme::Dark ? ThemeMode::Dark
                                                       : ThemeMode::Light,
      .activities = BuildGalleryActivities(state),
      .panel = audits[state.panel_audit_index],
  };
  bool return_requested = false;
  const FrameResult result =
      ui.DrawPanelAudit(view, [&assets, &state, &return_requested]() {
        DrawPanelAuditMenu(assets, audits, state, return_requested);
      });
  if (result.navigation_changed) {
    const Destination destination = ui.session().active_destination;
    const auto selected =
        std::ranges::find(audits, destination, &PanelContractView::destination);
    if (selected != audits.end()) {
      state.panel_audit_index =
          static_cast<std::size_t>(std::distance(audits.begin(), selected));
    }
  }
  state.shell.layout.explorer_width = ui.session().explorer_width;
  state.shell.layout.inspector_width = ui.session().inspector_width;
  return return_requested;
}

} // namespace

bool DrawApplicationShellGallery(detail::UiAssetAtlas &assets,
                                 GalleryState &state) {
  if (state.active_tab == GalleryTab::PanelAudits) {
    return DrawPanelAuditGallery(assets, state);
  }
  static ApplicationUi ui(assets);
  static bool initialized = false;
  if (!initialized) {
    SessionState session{
        .active_destination =
            state.shell.active_workspace == WorkspaceKind::Canvas
                ? Destination::CanvasObjects
                : Destination::Model,
        .explorer_visible = state.shell.layout.explorer_visible,
        .inspector_visible = state.shell.layout.inspector_visible,
        .operation_tray_visible = state.shell.operation.expanded,
        .explorer_width = state.shell.layout.explorer_width,
        .inspector_width = state.shell.layout.inspector_width,
        .operation_tray_height = state.shell.operation.tray_height,
    };
    ui.SetSession(std::move(session));
    initialized = true;
  }

  ApplicationView view = BuildGalleryApplicationView(state);
  const FrameResult result = ui.Draw(view, {});
  bool return_requested = false;
  for (const UiIntent &intent : result.product_intents) {
    std::visit(
        [&view, &state, &return_requested](const auto &value) {
          using Intent = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<Intent, InvokeCommand>) {
            if (value.control.value == "gallery.back") {
              return_requested = true;
            } else if (const CommandView *command =
                           FindCommand(view, value.control, value.command)) {
              RecordShellCommandInvocation(state.shell, *command);
            }
          } else if constexpr (std::is_same_v<Intent, EditField>) {
            if (ApplyGalleryPanelEdit(state.shell, value)) {
              state.shell.feedback = "Updated the typed panel contract.";
            }
          } else {
            state.shell.feedback =
                "Selection intent: " + value.entity.value + ".";
          }
        },
        intent);
  }
  const SessionState &session = ui.session();
  state.shell.active_workspace =
      WorkspaceForDestination(session.active_destination);
  state.shell.layout.explorer_visible = session.explorer_visible;
  state.shell.layout.inspector_visible = session.inspector_visible;
  state.shell.layout.operation_tray_visible = session.operation_tray_visible;
  state.shell.layout.explorer_width = session.explorer_width;
  state.shell.layout.inspector_width = session.inspector_width;
  state.shell.layout.operation_tray_height = session.operation_tray_height;
  state.shell.operation.expanded = session.operation_tray_visible;
  state.shell.operation.tray_height = session.operation_tray_height;
  state.shell.model_toolbar.grid_target = session.model_grid_target.value;
  return return_requested;
}

} // namespace fancy_ui::gallery
