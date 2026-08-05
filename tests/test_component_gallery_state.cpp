#include "component_gallery.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

using namespace fancy_ui::gallery;

TEST_CASE("gallery tab names select deterministic state sheets") {
  REQUIRE(ParseGalleryTab("components") == GalleryTab::Components);
  REQUIRE(ParseGalleryTab("shell") == GalleryTab::Shell);
  REQUIRE(ParseGalleryTab("settings") == GalleryTab::Settings);
  REQUIRE(ParseGalleryTab("operations") == GalleryTab::Operations);
  REQUIRE(ParseGalleryTab("status") == GalleryTab::Status);
  REQUIRE_FALSE(ParseGalleryTab("operation"));
}

TEST_CASE("shell preview returns to the tab that opened it") {
  GalleryState state;
  ActivateGalleryTab(state, GalleryTab::Settings);
  ActivateGalleryTab(state, GalleryTab::Shell);

  REQUIRE(state.active_tab == GalleryTab::Shell);
  REQUIRE(state.shell_return_tab == GalleryTab::Settings);

  ActivateGalleryTab(state, GalleryTab::Shell);
  REQUIRE(state.shell_return_tab == GalleryTab::Settings);

  LeaveShellPreview(state);
  REQUIRE(state.active_tab == GalleryTab::Settings);
  REQUIRE(state.focus_active_tab);
}

TEST_CASE("command-line shell startup returns to Components") {
  GalleryState state;
  ActivateGalleryTab(state, GalleryTab::Shell);
  LeaveShellPreview(state);

  REQUIRE(state.active_tab == GalleryTab::Components);
  REQUIRE(state.focus_active_tab);
}

TEST_CASE("gallery screenshots use state-sheet-specific logical extents") {
  const GalleryCaptureExtent components =
      GalleryScreenshotLogicalExtent(GalleryTab::Components);
  const GalleryCaptureExtent shell =
      GalleryScreenshotLogicalExtent(GalleryTab::Shell);
  const GalleryCaptureExtent settings =
      GalleryScreenshotLogicalExtent(GalleryTab::Settings);
  const GalleryCaptureExtent operations =
      GalleryScreenshotLogicalExtent(GalleryTab::Operations);
  const GalleryCaptureExtent status =
      GalleryScreenshotLogicalExtent(GalleryTab::Status);

  REQUIRE(components.width == 1280);
  REQUIRE(components.height == 1320);
  REQUIRE(shell.width == 1280);
  REQUIRE(shell.height == 720);
  REQUIRE(settings.width == 1280);
  REQUIRE(settings.height == 1440);
  REQUIRE(operations.width == 1280);
  REQUIRE(operations.height == 1440);
  REQUIRE(status.width == 1280);
  REQUIRE(status.height == 1440);
}

TEST_CASE("operation phases choose canonical default tray disclosure") {
  for (const OperationPhase phase :
       std::array{OperationPhase::Preview, OperationPhase::Running,
                  OperationPhase::Stopping, OperationPhase::Finalizing}) {
    REQUIRE_FALSE(OperationDetailsExpandedByDefault(phase));
  }
  for (const OperationPhase phase :
       std::array{OperationPhase::Paused, OperationPhase::Completed,
                  OperationPhase::Failed}) {
    REQUIRE(OperationDetailsExpandedByDefault(phase));
  }

  const auto states = DefaultOperationPresentationStates();
  REQUIRE_FALSE(states[1].expanded);
  REQUIRE(states[2].expanded);
  REQUIRE(states[5].expanded);
  REQUIRE(states[6].expanded);
  REQUIRE(states[7].tray_height == kOperationTrayMaximumHeight);
}

TEST_CASE("operation tray resizing clamps and follows keyboard steps") {
  REQUIRE(ClampOperationTrayHeight(80.0f) == kOperationTrayMinimumHeight);
  REQUIRE(ClampOperationTrayHeight(999.0f) == kOperationTrayMaximumHeight);
  REQUIRE(OperationTrayHeightAfterCommand(
              160.0f, TrayResizeCommand::Increase) == 168.0f);
  REQUIRE(OperationTrayHeightAfterCommand(
              168.0f, TrayResizeCommand::Decrease) == 160.0f);
  REQUIRE(OperationTrayHeightAfterCommand(200.0f, TrayResizeCommand::Minimum) ==
          kOperationTrayMinimumHeight);
  REQUIRE(OperationTrayHeightAfterCommand(200.0f, TrayResizeCommand::Maximum) ==
          kOperationTrayMaximumHeight);
}

TEST_CASE("operation strip height is stable across tray disclosure") {
  const OperationLayout collapsed =
      ResolveOperationLayout(false, true, 200.0f, false);
  const OperationLayout expanded =
      ResolveOperationLayout(true, true, 200.0f, false);
  const OperationLayout without_details =
      ResolveOperationLayout(true, false, 200.0f, false);
  const OperationLayout with_feedback =
      ResolveOperationLayout(false, true, 200.0f, true);

  REQUIRE(collapsed.strip_height == kOperationStripHeight);
  REQUIRE(expanded.strip_height == kOperationStripHeight);
  REQUIRE(expanded.content_height - collapsed.content_height == 200.0f);
  REQUIRE(without_details.tray_height == 0.0f);
  REQUIRE(with_feedback.content_height - collapsed.content_height ==
          kOperationFeedbackHeight);
}

TEST_CASE("status zoom uses a logarithmic ten to sixteen hundred scale") {
  REQUIRE(StatusZoomPercentFromSliderPosition(0.0f) ==
          kStatusZoomMinimumPercent);
  REQUIRE(StatusZoomPercentFromSliderPosition(100.0f) ==
          kStatusZoomMaximumPercent);
  REQUIRE(StatusZoomSliderPositionFromPercent(-50.0f) == Catch::Approx(0.0f));
  REQUIRE(StatusZoomSliderPositionFromPercent(5000.0f) ==
          Catch::Approx(100.0f));

  for (const float percent :
       std::array{10.0f, 25.0f, 50.0f, 100.0f, 240.0f, 800.0f, 1600.0f}) {
    const float position = StatusZoomSliderPositionFromPercent(percent);
    const float round_trip = StatusZoomPercentFromSliderPosition(position);
    REQUIRE(round_trip ==
            Catch::Approx(percent).margin(std::max(2.0f, percent * 0.03f)));
  }
}

TEST_CASE("application shell gallery starts with both side panels visible") {
  ShellGalleryState state;
  REQUIRE(state.layout.explorer_visible);
  REQUIRE(state.layout.inspector_visible);
  REQUIRE(state.operation.expanded);
  REQUIRE(state.layout.operation_tray_visible);
  REQUIRE(state.has_selection);
  REQUIRE(state.has_model);
  REQUIRE(state.has_assigned_selection);
  REQUIRE(state.can_convert_to_partbed);
  REQUIRE(state.active_workspace ==
          fancy_ui::steppenface::WorkspaceKind::Canvas);
  REQUIRE(state.layout.explorer_width == 256.0f);
  REQUIRE(state.layout.inspector_width == 320.0f);
  REQUIRE(state.canvas_toolbar.selection_scope ==
          fancy_ui::steppenface::SelectionScope::Canvas);
  REQUIRE(state.canvas_toolbar.selection_tool ==
          fancy_ui::steppenface::SelectionTool::Pointer);
  REQUIRE(state.canvas_toolbar.grid_visible);
  REQUIRE(state.canvas_toolbar.grid_spacing_mm == 10.0);
  REQUIRE(state.model_toolbar.grid_target == "all");
  REQUIRE(state.model_toolbar.beds[0].grid_spacing_mm == 25);
  REQUIRE(state.model_toolbar.beds[1].snap_to_grid);
}

TEST_CASE("component gallery starts in the canonical mockup selections") {
  const GalleryState state;

  REQUIRE(state.component_workspace ==
          fancy_ui::steppenface::WorkspaceKind::Model3d);
  REQUIRE(state.component_selection_scope ==
          fancy_ui::steppenface::SelectionScope::Canvas);
  REQUIRE(state.component_selection_tool ==
          fancy_ui::steppenface::SelectionTool::Pointer);
  REQUIRE(state.rotations == 4);
}

TEST_CASE("gallery shell merge preserves application bar visibility changes") {
  ShellGalleryState state;
  state.operation.expanded = false;
  state.operation.tray_height = 216.0f;
  fancy_ui::shell::ApplicationShellState stale_result = state.layout;
  stale_result.explorer_visible = true;
  stale_result.inspector_visible = true;
  stale_result.operation_tray_visible = true;
  stale_result.explorer_width = 272.0f;
  stale_result.inspector_width = 344.0f;
  stale_result.operation_tray_height = 184.0f;

  MergeGalleryShellResult(state, stale_result, false, false);

  REQUIRE_FALSE(state.layout.explorer_visible);
  REQUIRE_FALSE(state.layout.inspector_visible);
  REQUIRE_FALSE(state.layout.operation_tray_visible);
  REQUIRE(state.layout.explorer_width == 272.0f);
  REQUIRE(state.layout.inspector_width == 344.0f);
  REQUIRE(state.layout.operation_tray_height == 216.0f);
}

TEST_CASE("gallery Canvas toolbar actions retain exact view settings") {
  using fancy_ui::steppenface::ControlActionView;
  using fancy_ui::steppenface::SelectionScope;
  using fancy_ui::steppenface::SelectionTool;

  ShellGalleryState state;
  const auto action = [](std::string field,
                         fancy_ui::steppenface::FieldValue value) {
    return ControlActionView{
        .field = {.value = std::move(field)},
        .value = std::move(value),
    };
  };

  REQUIRE(ApplyGalleryToolbarAction(
      state, action("canvas.selection-scope", SelectionScope::Object)));
  REQUIRE(ApplyGalleryToolbarAction(
      state, action("canvas.selection-tool", SelectionTool::Oval)));
  REQUIRE(
      ApplyGalleryToolbarAction(state, action("canvas.grid-visible", false)));
  REQUIRE(ApplyGalleryToolbarAction(
      state, action("canvas.grid-spacing", std::int64_t{25})));
  REQUIRE(
      ApplyGalleryToolbarAction(state, action("canvas.snap.minor-grid", true)));

  REQUIRE(state.canvas_toolbar.selection_scope == SelectionScope::Object);
  REQUIRE(state.canvas_toolbar.selection_tool == SelectionTool::Oval);
  REQUIRE_FALSE(state.canvas_toolbar.grid_visible);
  REQUIRE(state.canvas_toolbar.grid_spacing_mm == 25.0);
  REQUIRE(state.canvas_toolbar.snap_minor_grid);

  REQUIRE_FALSE(
      ApplyGalleryToolbarAction(state, action("canvas.grid-spacing", -1.0)));
  REQUIRE(state.canvas_toolbar.grid_spacing_mm == 25.0);
  REQUIRE(state.feedback == "Grid spacing must be greater than zero.");
}

TEST_CASE("gallery model grid actions honor all-bed and named targets") {
  using fancy_ui::steppenface::ControlActionView;
  using fancy_ui::steppenface::FieldValue;

  ShellGalleryState state;
  const auto action = [](std::string field, FieldValue value,
                         std::string target = {}) {
    return ControlActionView{
        .field = {.value = std::move(field)},
        .value = std::move(value),
        .target =
            target.empty()
                ? std::optional<fancy_ui::steppenface::UiId>{}
                : std::optional<
                      fancy_ui::steppenface::UiId>{fancy_ui::steppenface::UiId{
                      .value = std::move(target)}},
    };
  };

  REQUIRE(ApplyGalleryToolbarAction(
      state, action("session.model-grid-target", std::string{"bed.2"})));
  REQUIRE(ApplyGalleryToolbarAction(
      state, action("model.grid-spacing", std::int64_t{50}, "bed.2")));
  REQUIRE(ApplyGalleryToolbarAction(state, action("model.snap", false, "all")));

  REQUIRE(state.model_toolbar.grid_target == "bed.2");
  REQUIRE(state.model_toolbar.beds[0].grid_spacing_mm == 25);
  REQUIRE(state.model_toolbar.beds[1].grid_spacing_mm == 50);
  REQUIRE_FALSE(state.model_toolbar.beds[0].snap_to_grid);
  REQUIRE_FALSE(state.model_toolbar.beds[1].snap_to_grid);

  REQUIRE_FALSE(ApplyGalleryToolbarAction(
      state, action("model.grid-spacing", std::int64_t{0}, "all")));
  REQUIRE(state.model_toolbar.beds[0].grid_spacing_mm == 25);
  REQUIRE(state.model_toolbar.beds[1].grid_spacing_mm == 50);
}

TEST_CASE("gallery application commands report typed interaction feedback") {
  ShellGalleryState state;
  const fancy_ui::steppenface::CommandView command{
      .id = {.value = "view.zoom-fit"},
      .command = fancy_ui::steppenface::CommandId::ZoomToFit,
      .label = "Zoom to fit",
  };

  RecordShellCommandInvocation(state, command);

  REQUIRE(state.feedback == "Application command invoked: Zoom to fit.");
}

TEST_CASE("gallery unavailable commands preserve backend capability identity") {
  using fancy_ui::steppenface::BackendCapability;
  using fancy_ui::steppenface::CommandId;

  REQUIRE(GalleryMissingBackendCapability(CommandId::OpenProject) ==
          BackendCapability::ProjectPersistence);
  REQUIRE(GalleryMissingBackendCapability(CommandId::SaveProject) ==
          BackendCapability::ProjectPersistence);
  REQUIRE(GalleryMissingBackendCapability(CommandId::SaveProjectAs) ==
          BackendCapability::ProjectPersistence);
  REQUIRE(GalleryMissingBackendCapability(CommandId::ExportFile) ==
          BackendCapability::ExportJob);
  REQUIRE(GalleryMissingBackendCapability(CommandId::OpenSettings) ==
          BackendCapability::SettingsPersistence);
  REQUIRE(GalleryMissingBackendCapability(CommandId::OpenLicense) ==
          BackendCapability::LicenseManagement);
  REQUIRE(GalleryMissingBackendCapability(CommandId::OpenLegalNotices) ==
          BackendCapability::None);
}
