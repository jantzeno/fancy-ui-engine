#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/application_chrome.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstdint>
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

UiAvailability Disabled(std::string reason) {
  return {
      .enabled = false,
      .disabled_reason = std::move(reason),
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

ApplicationBarView BuildGalleryApplicationBar(const WorkspaceKind workspace) {
  const UiAvailability needs_selection =
      Disabled("Select an object or model item first.");
  const UiAvailability canvas_only =
      workspace == WorkspaceKind::Canvas
          ? Enabled()
          : Disabled("Available in the Canvas workspace.");
  ApplicationBarView bar{
      .active_workspace = workspace,
      .document_dirty = true,
      .dirty_label = "Unsaved",
  };
  bar.menus = {
      Menu("menu.file", "File",
           {
               MenuCommand(GalleryCommand(CommandId::OpenFile, "file.open",
                                          "Open File…", "Ctrl+O")),
               MenuCommand(GalleryCommand(
                   CommandId::ExportFile, "file.export", "Export…", {},
                   Disabled("Export jobs are not implemented."))),
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
                   "Select external faces", {},
                   Disabled("Available in the 3D Model activity with a "
                            "loaded model."))),
               MenuCommand(GalleryCommand(
                   CommandId::SelectInternalFaces, "edit.select-internal",
                   "Select internal faces", {},
                   Disabled("Available in the 3D Model activity with a "
                            "loaded model."))),
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
                                          needs_selection)),
               MenuCommand(GalleryCommand(CommandId::UngroupSelection,
                                          "object.ungroup", "Ungroup",
                                          "Ctrl+Shift+G", needs_selection)),
               MenuCommand(GalleryCommand(CommandId::ExtractSelection,
                                          "object.extract", "Extract", {},
                                          needs_selection)),
               MenuSeparator("object.separator.transform"),
               MenuSubmenu(
                   "object.transform", "Transform",
                   {
                       MenuCommand(GalleryCommand(
                           CommandId::FlipHorizontal, "object.flip-horizontal",
                           "Flip horizontal", {}, needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::FlipVertical, "object.flip-vertical",
                           "Flip vertical", {}, needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::RotateCW, "object.rotate-clockwise",
                           "Rotate clockwise", {}, needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::RotateCCW,
                           "object.rotate-counterclockwise",
                           "Rotate counterclockwise", {}, needs_selection)),
                   }),
               MenuSubmenu("object.assignment", "Assignment",
                           {
                               MenuCommand(GalleryCommand(
                                   CommandId::ConvertObjectToPartBed,
                                   "object.convert-to-bed",
                                   "Convert to part bed", {}, needs_selection)),
                               MenuCommand(GalleryCommand(
                                   CommandId::ClearAssignment,
                                   "object.clear-assignment",
                                   "Clear assignment", {}, needs_selection)),
                           }),
               MenuSeparator("object.separator.delete"),
               MenuCommand(GalleryCommand(
                   CommandId::DeleteObjects, "object.delete", "Delete",
                   "Delete", needs_selection, CommandVariant::Destructive)),
           }),
      Menu("menu.tools", "Tools",
           {
               MenuCommand(GalleryCommand(CommandId::AnalyzeContours,
                                          "tools.analyze", "Analyze contours",
                                          {}, needs_selection)),
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
                   "Convert object to part bed", {}, needs_selection)),
               MenuSubmenu(
                   "tools.deconstruction", "Deconstruction",
                   {
                       MenuCommand(GalleryCommand(
                           CommandId::ClearPreview, "tools.clear-preview",
                           "Clear preview", {}, canvas_only)),
                       MenuCommand(GalleryCommand(
                           CommandId::PreviewGuideSplit, "tools.preview-guide",
                           "Preview guide split", {}, needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::ApplyGuideSplit, "tools.apply-guide",
                           "Apply guide split", {}, needs_selection)),
                       MenuCommand(
                           GalleryCommand(CommandId::PreviewAutoSplitHorizontal,
                                          "tools.preview-auto-horizontal",
                                          "Preview horizontal auto-split", {},
                                          needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::PreviewAutoSplitVertical,
                           "tools.preview-auto-vertical",
                           "Preview vertical auto-split", {}, needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::PreviewAutoSplitBoth,
                           "tools.preview-auto-both", "Preview both directions",
                           {}, needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::ApplyAutoSplit, "tools.apply-auto",
                           "Apply auto-split", {}, needs_selection)),
                   }),
               MenuSubmenu(
                   "tools.repair", "Repair",
                   {
                       MenuCommand(GalleryCommand(
                           CommandId::JoinOpenSegments, "tools.join-open",
                           "Join open segments", {}, needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::RepairOrphanHoles, "tools.repair-orphan",
                           "Repair orphan holes", {}, needs_selection)),
                       MenuCommand(GalleryCommand(
                           CommandId::ReindexObjects, "tools.reindex",
                           "Reindex objects", {}, canvas_only)),
                   }),
               MenuSeparator("tools.separator.settings"),
               MenuCommand(GalleryCommand(
                   CommandId::OpenSettings, "tools.settings", "Settings…",
                   "Ctrl+,",
                   Disabled("Settings persistence is not implemented."))),
           }),
      Menu("menu.help", "Help",
           {
               MenuCommand(GalleryCommand(
                   CommandId::OpenLicense, "help.license", "License…", {},
                   Disabled("License management is not implemented."))),
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

ContextToolbarView BuildGalleryContextToolbar(const WorkspaceKind workspace) {
  ContextToolbarView toolbar;
  if (workspace == WorkspaceKind::Model3d) {
    toolbar.items.emplace_back(ToolbarSegmentedView{
        .id = {.value = "model.selection-tool"},
        .choices =
            {
                {.id = {.value = "model.pointer"},
                 .label = "Pointer",
                 .tooltip = "Select model faces with the pointer",
                 .selected = true,
                 .action = GalleryAction("session.model-selection-tool",
                                         std::string{"pointer"})},
                {.id = {.value = "model.rectangle"},
                 .label = "Rectangle",
                 .tooltip = "Select enclosed visible parts",
                 .action = GalleryAction("session.model-selection-tool",
                                         std::string{"rectangle"})},
            },
    });
    toolbar.items.emplace_back(
        ToolbarSeparatorView{.id = {.value = "model.selection-separator"}});
    toolbar.items.emplace_back(GalleryCommand(
        CommandId::SelectExternalFaces, "model.select-external",
        "Select external faces", {},
        Disabled("Available in the Model activity when a model is loaded.")));
    toolbar.items.emplace_back(GalleryCommand(
        CommandId::SelectInternalFaces, "model.select-internal",
        "Select internal faces", {},
        Disabled("Available in the Model activity when a model is loaded.")));
    toolbar.items.emplace_back(GalleryCommand(
        CommandId::ClearSelection, "model.clear-selection", "Clear selection",
        {}, Disabled("No model selection to clear."),
        CommandVariant::Tertiary));
    toolbar.items.emplace_back(
        ToolbarSpacerView{.id = {.value = "model.toolbar-spacer"}});
    toolbar.items.emplace_back(ToolbarPopoverView{
        .id = {.value = "model.grid"},
        .label = "Grid: 10 mm",
        .tooltip = "Set exact grid spacing",
        .availability = Enabled(),
        .items =
            {
                {.id = {.value = "grid.10"},
                 .label = "All beds",
                 .secondary_label = "10 mm",
                 .selected = true,
                 .action =
                     GalleryAction("model.grid-spacing", std::int64_t{10})},
            },
    });
    toolbar.items.emplace_back(ToolbarPopoverView{
        .id = {.value = "model.snap"},
        .label = "Snap: On",
        .tooltip = "Set grid snapping",
        .availability = Enabled(),
        .items =
            {
                {.id = {.value = "snap.on"},
                 .label = "All beds",
                 .secondary_label = "On",
                 .selected = true,
                 .action = GalleryAction("model.grid-snap", true)},
            },
    });
    return toolbar;
  }

  toolbar.items.emplace_back(ToolbarSegmentedView{
      .id = {.value = "canvas.selection-scope"},
      .choices =
          {
              {.id = {.value = "canvas.scope.canvas"},
               .label = "Canvas",
               .tooltip = "Select Canvas-level content",
               .selected = true,
               .action = GalleryAction("session.canvas-selection-scope",
                                       std::string{"canvas"})},
              {.id = {.value = "canvas.scope.object"},
               .label = "Object",
               .tooltip = "Select object content",
               .action = GalleryAction("session.canvas-selection-scope",
                                       std::string{"object"})},
          },
  });
  toolbar.items.emplace_back(
      ToolbarSeparatorView{.id = {.value = "canvas.scope-separator"}});
  toolbar.items.emplace_back(ToolbarSegmentedView{
      .id = {.value = "canvas.selection-tool"},
      .choices =
          {
              {.id = {.value = "canvas.pointer"},
               .label = "Pointer",
               .tooltip = "Select with the pointer",
               .selected = true,
               .action = GalleryAction("session.canvas-selection-tool",
                                       std::string{"pointer"})},
              {.id = {.value = "canvas.rectangle"},
               .label = "Rectangle",
               .tooltip = "Select within a rectangle",
               .action = GalleryAction("session.canvas-selection-tool",
                                       std::string{"rectangle"})},
              {.id = {.value = "canvas.oval"},
               .label = "Oval",
               .tooltip = "Select within an oval",
               .action = GalleryAction("session.canvas-selection-tool",
                                       std::string{"oval"})},
          },
  });
  toolbar.items.emplace_back(
      ToolbarSeparatorView{.id = {.value = "canvas.tool-separator"}});
  toolbar.items.emplace_back(GalleryCommand(
      CommandId::ClearSelection, "canvas.clear-selection", "Clear selection",
      {}, Disabled("No Canvas selection to clear."), CommandVariant::Tertiary));
  toolbar.items.emplace_back(
      ToolbarSpacerView{.id = {.value = "canvas.toolbar-spacer"}});
  toolbar.items.emplace_back(ToolbarPopoverView{
      .id = {.value = "canvas.grid"},
      .label = "Grid: 10 mm",
      .tooltip = "Set Canvas grid spacing",
      .availability = Enabled(),
      .items =
          {
              {.id = {.value = "canvas.grid.10"},
               .label = "Grid spacing",
               .secondary_label = "10 mm",
               .selected = true,
               .action =
                   GalleryAction("canvas.grid-spacing", std::int64_t{10})},
          },
  });
  toolbar.items.emplace_back(ToolbarPopoverView{
      .id = {.value = "canvas.snap"},
      .label = "Snap: On",
      .tooltip = "Set Canvas grid snapping",
      .availability = Enabled(),
      .items =
          {
              {.id = {.value = "canvas.snap.on"},
               .label = "Snap to grid",
               .secondary_label = "On",
               .selected = true,
               .action = GalleryAction("canvas.grid-snap", true)},
          },
  });
  return toolbar;
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

void DrawWorkspace(detail::UiAssetAtlas &assets) {
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
      ImGui::TextDisabled("%s", shell.feedback.c_str());
    }
    EndSection(feedback);
  }
  ImGui::EndChild();
}

} // namespace

void DrawApplicationShellGallery(detail::UiAssetAtlas &assets,
                                 GalleryState &state) {
  ImGui::TextDisabled(
      "Canonical nine-region shell using the production application menu and "
      "context toolbar; both tray controls share one state.");
  ImGui::Spacing();
  if (ImGui::BeginChild("##application-shell-gallery", ImVec2(0.0f, 0.0f),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoSavedSettings)) {
    detail::ApplicationChrome chrome(assets);
    ApplicationBarView application_bar =
        BuildGalleryApplicationBar(state.shell.active_workspace);
    ContextToolbarView context_toolbar =
        BuildGalleryContextToolbar(state.shell.active_workspace);
    const detail::ApplicationChromeCallbacks chrome_callbacks{
        .activate_workspace =
            [&state](const WorkspaceKind workspace) {
              state.shell.active_workspace = workspace;
            },
        .toggle_layout =
            [&state](const detail::LayoutRegion region) {
              switch (region) {
              case detail::LayoutRegion::Explorer:
                state.shell.layout.explorer_visible =
                    !state.shell.layout.explorer_visible;
                break;
              case detail::LayoutRegion::OperationTray:
                state.shell.operation.expanded =
                    !state.shell.operation.expanded;
                state.shell.layout.operation_tray_visible =
                    state.shell.operation.expanded;
                break;
              case detail::LayoutRegion::Inspector:
                state.shell.layout.inspector_visible =
                    !state.shell.layout.inspector_visible;
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
                    [&chrome, &application_bar, &state, &chrome_callbacks]() {
                      chrome.DrawApplicationBar(
                          application_bar,
                          {
                              .explorer_visible =
                                  state.shell.layout.explorer_visible,
                              .operation_tray_visible =
                                  state.shell.operation.expanded,
                              .operation_available = true,
                              .inspector_visible =
                                  state.shell.layout.inspector_visible,
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
                .draw = [&assets]() { DrawWorkspace(assets); },
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
    const shell::ApplicationShellResult result =
        shell::Application(spec, input);
    ImGui::PopStyleVar();
    state.shell.layout = result.state;
    state.shell.layout.operation_tray_height =
        state.shell.operation.tray_height;
    state.shell.layout.operation_tray_visible = state.shell.operation.expanded;
  }
  ImGui::EndChild();
}

} // namespace fancy_ui::gallery
