#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/application_chrome.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
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
  popover.fields.push_back({
      .id = {.value = "model.grid-spacing"},
      .label = "Custom spacing",
      .value = static_cast<std::int64_t>(current_spacing),
      .target = UiId{.value = target},
      .unit = "mm",
      .help = "Enter a positive whole-millimeter spacing.",
  });
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
  popover.fields.push_back({
      .id = {.value = "canvas.grid-spacing"},
      .label = "Custom spacing",
      .value = state.grid_spacing_mm,
      .unit = "mm",
      .help = "Enter a positive grid spacing.",
  });
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

void DrawGalleryToolbarField(ShellGalleryState &state, const FieldView &field) {
  if (!field.availability.visible) {
    return;
  }
  ImGui::PushID(field.id.value.c_str());
  ImGui::BeginDisabled(!field.availability.enabled || field.availability.busy);
  bool changed = false;
  FieldValue edited = field.value;
  if (std::int64_t *value = std::get_if<std::int64_t>(&edited)) {
    changed = ImGui::InputScalar(field.label.c_str(), ImGuiDataType_S64, value);
  } else if (double *value = std::get_if<double>(&edited)) {
    changed = ImGui::InputDouble(field.label.c_str(), value, 0.0, 0.0, "%.3f");
  }
  ImGui::EndDisabled();
  if (changed) {
    static_cast<void>(
        ApplyGalleryToolbarAction(state, {
                                             .field = field.id,
                                             .value = std::move(edited),
                                             .target = field.target,
                                             .availability = field.availability,
                                         }));
  }
  if (!field.help.empty()) {
    detail::DrawSecondaryText(field.help);
  }
  ImGui::PopID();
}

void DrawPanelHeading(detail::UiAssetAtlas &assets,
                      const std::string_view title) {
  if (assets.bold_font() != nullptr) {
    ImGui::PushFont(assets.bold_font());
  }
  ImGui::TextUnformatted(title.data(), title.data() + title.size());
  if (assets.bold_font() != nullptr) {
    ImGui::PopFont();
  }
  ImGui::Separator();
}

void DrawActivityRail(detail::UiAssetAtlas &assets,
                      const bool diagnostics_enabled) {
  struct Activity {
    std::string_view id;
    std::string_view label;
    std::string_view icon;
  };
  static constexpr std::array activities{
      Activity{"objects", "Objects", "objects"},
      Activity{"beds", "Beds", "bed"},
      Activity{"grain", "Grain", "grain"},
      Activity{"search", "Search", "search"},
      Activity{"compact", "Compact", "compact"},
  };
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
  for (const Activity &activity : activities) {
    static_cast<void>(NavigationItem({
        .id = activity.id,
        .label = activity.label,
        .tooltip = activity.label,
        .selected = activity.id == "objects",
        .draw_icon = assets.Painter(activity.icon),
    }));
  }
  if (diagnostics_enabled) {
    static_cast<void>(NavigationItem({
        .id = "diagnostics",
        .label = "Diagnostics",
        .tooltip = "Diagnostics",
        .draw_icon = assets.Painter("diagnostics"),
    }));
  }
  ImGui::PopStyleVar();
}

void DrawExplorer(detail::UiAssetAtlas &assets, GalleryState &state) {
  DrawPanelHeading(assets, "Objects");
  static_cast<void>(TextInput({
      .id = "shell-explorer-search",
      .label = "Search",
      .value = "",
  }));
  ImGui::Spacing();
  if (ImGui::BeginChild("##shell-hierarchy", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
    DrawHierarchySample(assets, state);
  }
  ImGui::EndChild();
}

[[nodiscard]] bool DrawWorkspace(detail::UiAssetAtlas &assets,
                                 const std::string_view feedback) {
  const float available_height = ImGui::GetContentRegionAvail().y;
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                       std::max(Scale(24.0f), available_height * 0.38f));
  EmptyState({
      .id = "blank-workspace",
      .title = "Blank view",
      .message = "No surface is bound in this gallery composition.",
      .icon = assets.Painter("objects"),
      .minimum_height = 96.0f,
  });
  ImGui::Spacing();
  const float button_width = Scale(220.0f);
  const float hint_width = ImGui::CalcTextSize("Esc").x;
  const float row_width =
      button_width + ImGui::GetStyle().ItemSpacing.x + hint_width;
  ImGui::SetCursorPosX(
      ImGui::GetCursorPosX() +
      std::max(0.0f, (ImGui::GetContentRegionAvail().x - row_width) * 0.5f));
  const bool return_requested =
      Button({
                 .id = "back-to-component-gallery",
                 .label = "Back to component gallery",
                 .variant = ButtonVariant::Tertiary,
                 .size = {.x = button_width, .y = Scale(32.0f)},
             })
          .activated;
  ImGui::SameLine();
  detail::DrawSecondaryText("Esc");
  if (!feedback.empty()) {
    const float feedback_width =
        ImGui::CalcTextSize(feedback.data(), feedback.data() + feedback.size())
            .x;
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() +
        std::max(0.0f,
                 (ImGui::GetContentRegionAvail().x - feedback_width) * 0.5f));
    detail::DrawSecondaryText(feedback);
  }
  return return_requested;
}

void DrawInspector(detail::UiAssetAtlas &assets, GalleryState &state) {
  ShellGalleryState &shell = state.shell;
  DrawPanelHeading(assets, "Inspector");

  if (ImGui::BeginChild("##shell-inspector-scroll", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
    const SectionResult values = BeginSection({
        .id = "inspector-values",
        .heading = "Values and fields",
    });
    if (values.visible) {
      static_cast<void>(ValueDisplay({
          .id = "source",
          .label = "Source",
          .value = "fixture-kit-07.svg",
      }));
      const TextInputResult name = TextInput({
          .id = "name",
          .label = "Name",
          .value = shell.object_name,
      });
      if (name.changed) {
        shell.object_name = name.value;
      }
      const NumericInputResult spacing = NumericInput({
          .id = "spacing",
          .label = "Spacing",
          .unit = "mm",
          .value = shell.spacing_mm,
          .minimum = 0.0,
          .format = "%.1f",
      });
      if (spacing.changed) {
        shell.spacing_mm = spacing.value;
      }
      static constexpr std::array materials{
          SelectOption{.id = "plywood", .label = "Plywood"},
          SelectOption{.id = "acrylic", .label = "Acrylic"},
          SelectOption{.id = "aluminum", .label = "Aluminum"},
      };
      const SelectResult material = Select({
          .id = "material",
          .label = "Material",
          .options = materials,
          .selected_index = shell.material_index,
      });
      if (material.changed) {
        shell.material_index = material.selected_index;
      }
      const DurationResult duration = Duration({
          .id = "duration",
          .label = "Time limit",
          .hours = shell.hours,
          .minutes = shell.minutes,
      });
      if (duration.changed) {
        shell.hours = duration.hours;
        shell.minutes = duration.minutes;
      }
    }
    EndSection(values);

    const SectionResult toggles = BeginSection({
        .id = "inspector-toggles",
        .heading = "Toggles and range",
    });
    if (toggles.visible) {
      const CheckboxResult enabled = Checkbox({
          .id = "enabled",
          .label = "Enabled",
          .state = shell.enabled,
      });
      if (enabled.changed) {
        shell.enabled = enabled.state;
      }
      const VisibilityToggleResult visible = VisibilityToggle({
          .id = "visible",
          .label = "Show overlay",
          .state = shell.visible,
          .visible_icon = assets.Painter("visibility"),
          .hidden_icon = assets.Painter("visibility-off"),
      });
      if (visible.changed) {
        shell.visible = visible.state;
      }
      const SliderResult explode = Slider({
          .id = "explode",
          .label = "Explode",
          .unit = "%",
          .value = shell.explode_percent,
          .minimum = 0.0f,
          .maximum = 100.0f,
          .format = "%.0f",
      });
      if (explode.changed) {
        shell.explode_percent = explode.value;
      }
    }
    EndSection(toggles);

    const SectionResult specialized = BeginSection({
        .id = "inspector-specialized",
        .heading = "Specialized",
    });
    if (specialized.visible) {
      const RotationCompassResult rotations = RotationCompass({
          .id = "rotations",
          .label = "Search rotations",
          .count = shell.rotation_count,
      });
      if (rotations.changed) {
        shell.rotation_count = rotations.count;
      }
      const std::span<const ColorRgba> color(&shell.display_color, 1);
      const ColorSwatchResult swatch = ColorSwatch(
          {
              .id = "display-color",
              .label = "Display color",
              .tooltip = "Open display color picker",
              .picker_title = "Display color",
              .value = shell.display_color,
              .colors = color,
          },
          shell.color_picker);
      if (swatch.changed) {
        shell.display_color = swatch.value;
      }
    }
    EndSection(specialized);

    const SectionResult feedback = BeginSection({
        .id = "inspector-feedback",
        .heading = "Feedback and action",
    });
    if (feedback.visible) {
      StatusCard({
          .id = "readiness",
          .title = "Ready",
          .message = "Inspector components are ready.",
          .status = SemanticStatus::Success,
          .icon = assets.Painter("success"),
      });
      if (Button({
                     .id = "inspect",
                     .label = "Inspect component",
                     .variant = ButtonVariant::Primary,
                     .size = {.x = -1.0f, .y = 32.0f},
                 })
              .activated) {
        shell.feedback = "Component inspection requested.";
      }
      detail::DrawSecondaryText(shell.feedback);
    }
    EndSection(feedback);
  }
  ImGui::EndChild();
}

} // namespace

bool DrawApplicationShellGallery(detail::UiAssetAtlas &assets,
                                 GalleryState &state) {
  bool return_requested = false;
  detail::ApplicationChrome chrome(assets);
  ApplicationBarView application_bar = BuildGalleryApplicationBar(state.shell);
  ContextToolbarView context_toolbar = BuildGalleryContextToolbar(state.shell);
  bool explorer_visible = state.shell.layout.explorer_visible;
  bool inspector_visible = state.shell.layout.inspector_visible;
  const detail::ApplicationChromeCallbacks chrome_callbacks{
      .invoke_command =
          [&state](const CommandView &command) {
            RecordShellCommandInvocation(state.shell, command);
          },
      .commit_action =
          [&state](const ControlActionView &action) {
            static_cast<void>(ApplyGalleryToolbarAction(state.shell, action));
          },
      .draw_field =
          [&state](const FieldView &field) {
            DrawGalleryToolbarField(state.shell, field);
          },
      .activate_workspace =
          [&state](const WorkspaceKind workspace) {
            state.shell.active_workspace = workspace;
          },
      .toggle_layout =
          [&state, &explorer_visible,
           &inspector_visible](const detail::LayoutRegion region) {
            switch (region) {
            case detail::LayoutRegion::Explorer:
              explorer_visible = !explorer_visible;
              break;
            case detail::LayoutRegion::OperationTray:
              state.shell.operation.expanded = !state.shell.operation.expanded;
              break;
            case detail::LayoutRegion::Inspector:
              inspector_visible = !inspector_visible;
              break;
            }
          },
  };
  shell::ApplicationShellState input = state.shell.layout;
  input.operation_tray_visible = state.shell.operation.expanded;
  const shell::ApplicationShellSpec spec{
      .application_bar =
          {
              .id = "gallery-application-bar",
              .draw =
                  [&chrome, &application_bar, &state, &chrome_callbacks,
                   &explorer_visible, &inspector_visible]() {
                    chrome.DrawApplicationBar(
                        application_bar,
                        {
                            .explorer_visible = explorer_visible,
                            .operation_tray_visible =
                                state.shell.operation.expanded,
                            .operation_available = true,
                            .inspector_visible = inspector_visible,
                        },
                        chrome_callbacks,
                        detail::ApplicationBarHost::InlineRegion);
                  },
              .menu_bar = true,
              .zero_padding = true,
          },
      .context_toolbar =
          {
              .id = "gallery-context-toolbar",
              .draw =
                  [&chrome, &context_toolbar, &chrome_callbacks]() {
                    chrome.DrawContextToolbar(context_toolbar,
                                              chrome_callbacks);
                  },
              .zero_padding = true,
          },
      .activity_rail =
          {
              .id = "gallery-activity-rail",
              .draw =
                  [&assets, &state]() {
                    DrawActivityRail(
                        assets,
                        state.settings.applied.general.diagnostics_enabled);
                  },
              .zero_padding = true,
          },
      .explorer =
          {
              .id = "gallery-explorer",
              .draw = [&assets, &state]() { DrawExplorer(assets, state); },
          },
      .workspace =
          {
              .id = "gallery-workspace",
              .draw =
                  [&assets, &state, &return_requested]() {
                    return_requested =
                        DrawWorkspace(assets, state.shell.feedback) ||
                        return_requested;
                  },
          },
      .inspector =
          {
              .id = "gallery-inspector",
              .draw = [&assets, &state]() { DrawInspector(assets, state); },
          },
      .operation_tray =
          {
              .id = "gallery-operation-tray",
              .draw = [&assets,
                       &state]() { DrawShellOperationTray(assets, state); },
          },
      .operation_strip =
          {
              .id = "gallery-operation-strip",
              .draw = [&assets,
                       &state]() { DrawShellOperationStrip(assets, state); },
              .zero_padding = true,
          },
      .status_bar =
          {
              .id = "gallery-status-bar",
              .draw = [&assets,
                       &state]() { DrawShellStatusBar(assets, state); },
              .zero_padding = true,
          },
  };
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  const shell::ApplicationShellResult result = shell::Application(spec, input);
  ImGui::PopStyleVar();
  MergeGalleryShellResult(state.shell, result.state, explorer_visible,
                          inspector_visible);
  return return_requested;
}

} // namespace fancy_ui::gallery
