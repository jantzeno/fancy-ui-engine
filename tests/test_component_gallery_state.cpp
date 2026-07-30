#include "component_gallery.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>

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
  REQUIRE(components.height == 1200);
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

  REQUIRE(GalleryMissingBackendCapability(CommandId::ExportFile) ==
          BackendCapability::ExportJob);
  REQUIRE(GalleryMissingBackendCapability(CommandId::OpenSettings) ==
          BackendCapability::SettingsPersistence);
  REQUIRE(GalleryMissingBackendCapability(CommandId::OpenLicense) ==
          BackendCapability::LicenseManagement);
  REQUIRE(GalleryMissingBackendCapability(CommandId::OpenLegalNotices) ==
          BackendCapability::None);
}
