#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace fancy_ui::gallery {

enum class GalleryTab : std::uint8_t {
  Components,
  Shell,
  PanelAudits,
  Settings,
  Operations,
  Status,
};

inline constexpr int kGalleryTabCount = 6;

[[nodiscard]] constexpr bool IsFullCanvasPreview(const GalleryTab tab) {
  return tab == GalleryTab::Shell || tab == GalleryTab::PanelAudits;
}

struct GalleryCaptureExtent {
  int width = 1280;
  int height = 1440;
};

[[nodiscard]] constexpr GalleryCaptureExtent
GalleryScreenshotLogicalExtent(const GalleryTab tab) {
  switch (tab) {
  case GalleryTab::Components:
    return {.width = 1280, .height = 2760};
  case GalleryTab::Shell:
  case GalleryTab::PanelAudits:
    return {.width = 1280, .height = 720};
  case GalleryTab::Settings:
  case GalleryTab::Operations:
    return {.width = 1280, .height = 1440};
  case GalleryTab::Status:
    return {.width = 1280, .height = 1088};
  }
  return {};
}

enum class OperationPhase : std::uint8_t {
  Preview,
  Running,
  Paused,
  Stopping,
  Finalizing,
  Completed,
  Failed,
};

inline constexpr float kOperationTrayMinimumHeight = 160.0f;
inline constexpr float kOperationTrayMaximumHeight = 240.0f;
inline constexpr float kOperationTrayDefaultHeight = 160.0f;
inline constexpr float kGalleryStateHeadingHeight = 37.0f;
inline constexpr float kOperationStateHeadingHeight = 34.0f;
inline constexpr float kOperationStripHeight = 32.0f;
inline constexpr float kOperationStripItemHeight = 24.0f;
inline constexpr float kOperationFeedbackHeight = 24.0f;
inline constexpr std::size_t kOperationSampleCount = 8;
inline constexpr std::size_t kStatusSampleCount = 8;
inline constexpr float kStatusBarHeight = 24.0f;
inline constexpr float kStatusFactLabelGap = 4.0f;
inline constexpr float kStatusFactGroupGap = 12.0f;
inline constexpr float kStatusFactCellPadding = 4.0f;
inline constexpr float kStatusZoomPanelWidth = 230.0f;
inline constexpr float kStatusZoomCommandHeight = 22.0f;
inline constexpr float kStatusZoomCommandSpacing = 0.0f;
inline constexpr float kStatusZoomPanelPadding = 4.0f;
inline constexpr float kStatusZoomPanelItemSpacing = 2.0f;

struct OperationLayout {
  float heading_height = kOperationStateHeadingHeight;
  float tray_height = 0.0f;
  float strip_height = kOperationStripHeight;
  float feedback_height = 0.0f;
  float content_height = kOperationStateHeadingHeight + kOperationStripHeight;
};

struct OperationPresentationState {
  bool expanded = false;
  bool user_toggled = false;
  float tray_height = kOperationTrayDefaultHeight;
  std::string feedback;
};

struct StatusZoomPresentationState {
  bool open = false;
  bool request_focus = false;
  float percent = 100.0f;
  std::string feedback;
};

[[nodiscard]] constexpr bool
OperationDetailsExpandedByDefault(const OperationPhase phase) {
  switch (phase) {
  case OperationPhase::Paused:
  case OperationPhase::Completed:
  case OperationPhase::Failed:
    return true;
  case OperationPhase::Preview:
  case OperationPhase::Running:
  case OperationPhase::Stopping:
  case OperationPhase::Finalizing:
    return false;
  }
  return false;
}

[[nodiscard]] constexpr OperationLayout
ResolveOperationLayout(const bool expanded, const bool has_details,
                       const float tray_height, const bool has_feedback) {
  OperationLayout layout;
  if (expanded && has_details) {
    layout.tray_height = std::clamp(tray_height, kOperationTrayMinimumHeight,
                                    kOperationTrayMaximumHeight);
  }
  if (has_feedback) {
    layout.feedback_height = kOperationFeedbackHeight;
  }
  layout.content_height = layout.heading_height + layout.tray_height +
                          layout.strip_height + layout.feedback_height;
  return layout;
}

[[nodiscard]] inline std::optional<GalleryTab>
ParseGalleryTab(const std::string_view value) {
  if (value == "components") {
    return GalleryTab::Components;
  }
  if (value == "shell") {
    return GalleryTab::Shell;
  }
  if (value == "panel-audits") {
    return GalleryTab::PanelAudits;
  }
  if (value == "settings") {
    return GalleryTab::Settings;
  }
  if (value == "operations") {
    return GalleryTab::Operations;
  }
  if (value == "status") {
    return GalleryTab::Status;
  }
  return std::nullopt;
}

[[nodiscard]] constexpr std::array<OperationPresentationState,
                                   kOperationSampleCount>
DefaultOperationPresentationStates() {
  std::array<OperationPresentationState, kOperationSampleCount> states{};
  constexpr std::array phases{
      OperationPhase::Preview,    OperationPhase::Running,
      OperationPhase::Paused,     OperationPhase::Stopping,
      OperationPhase::Finalizing, OperationPhase::Completed,
      OperationPhase::Failed,     OperationPhase::Completed,
  };
  for (std::size_t index = 0; index < phases.size(); ++index) {
    states[index].expanded = OperationDetailsExpandedByDefault(phases[index]);
  }
  states[7].tray_height = kOperationTrayMaximumHeight;
  return states;
}

[[nodiscard]] inline std::array<StatusZoomPresentationState, kStatusSampleCount>
DefaultStatusZoomPresentationStates() {
  std::array<StatusZoomPresentationState, kStatusSampleCount> states{};
  states[1].open = true;
  states[1].percent = 240.0f;
  states[2].open = true;
  return states;
}

} // namespace fancy_ui::gallery
