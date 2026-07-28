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
  Settings,
  Operations,
  Status,
};

inline constexpr int kGalleryTabCount = 5;

enum class OperationPhase : std::uint8_t {
  Preview,
  Running,
  Paused,
  Stopping,
  Finalizing,
  Completed,
  Failed,
};

enum class TrayResizeCommand : std::uint8_t {
  Increase,
  Decrease,
  Minimum,
  Maximum,
};

inline constexpr float kOperationTrayMinimumHeight = 160.0f;
inline constexpr float kOperationTrayMaximumHeight = 240.0f;
inline constexpr float kOperationTrayDefaultHeight = 160.0f;
inline constexpr float kOperationTrayKeyboardStep = 8.0f;
inline constexpr float kGalleryStateHeadingHeight = 37.0f;
inline constexpr float kOperationStripHeight = 32.0f;
inline constexpr float kOperationStripItemHeight = 24.0f;
inline constexpr float kOperationFeedbackHeight = 24.0f;
inline constexpr std::size_t kOperationSampleCount = 8;
inline constexpr std::size_t kStatusSampleCount = 8;
inline constexpr float kStatusBarHeight = 24.0f;
inline constexpr float kStatusFactLabelGap = 8.0f;
inline constexpr float kStatusFactGroupGap = 16.0f;
inline constexpr float kStatusFactCellPadding = 8.0f;
inline constexpr float kStatusZoomCommandHeight = 32.0f;
inline constexpr float kStatusZoomCommandSpacing = 4.0f;
inline constexpr float kStatusZoomPanelPadding = 8.0f;
inline constexpr float kStatusZoomPanelItemSpacing = 8.0f;
inline constexpr float kStatusZoomMinimumPercent = 10.0f;
inline constexpr float kStatusZoomMaximumPercent = 1600.0f;

struct OperationLayout {
  float heading_height = kGalleryStateHeadingHeight;
  float tray_height = 0.0f;
  float strip_height = kOperationStripHeight;
  float feedback_height = 0.0f;
  float content_height = kGalleryStateHeadingHeight + kOperationStripHeight;
};

struct OperationPresentationState {
  bool expanded = false;
  bool user_toggled = false;
  float tray_height = kOperationTrayDefaultHeight;
  bool resizing = false;
  float resize_start_mouse_y = 0.0f;
  float resize_start_height = kOperationTrayDefaultHeight;
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

[[nodiscard]] constexpr float ClampOperationTrayHeight(const float height) {
  return std::clamp(height, kOperationTrayMinimumHeight,
                    kOperationTrayMaximumHeight);
}

[[nodiscard]] constexpr float
OperationTrayHeightAfterCommand(const float current,
                                const TrayResizeCommand command) {
  switch (command) {
  case TrayResizeCommand::Increase:
    return ClampOperationTrayHeight(current + kOperationTrayKeyboardStep);
  case TrayResizeCommand::Decrease:
    return ClampOperationTrayHeight(current - kOperationTrayKeyboardStep);
  case TrayResizeCommand::Minimum:
    return kOperationTrayMinimumHeight;
  case TrayResizeCommand::Maximum:
    return kOperationTrayMaximumHeight;
  }
  return ClampOperationTrayHeight(current);
}

[[nodiscard]] constexpr OperationLayout
ResolveOperationLayout(const bool expanded, const bool has_details,
                       const float tray_height, const bool has_feedback) {
  OperationLayout layout;
  if (expanded && has_details) {
    layout.tray_height = ClampOperationTrayHeight(tray_height);
  }
  if (has_feedback) {
    layout.feedback_height = kOperationFeedbackHeight;
  }
  layout.content_height = layout.heading_height + layout.tray_height +
                          layout.strip_height + layout.feedback_height;
  return layout;
}

[[nodiscard]] inline float
StatusZoomPercentFromSliderPosition(const float position) {
  const float normalized = std::clamp(position, 0.0f, 100.0f) / 100.0f;
  const float ratio = kStatusZoomMaximumPercent / kStatusZoomMinimumPercent;
  return std::round(kStatusZoomMinimumPercent * std::pow(ratio, normalized));
}

[[nodiscard]] inline float
StatusZoomSliderPositionFromPercent(const float percent) {
  const float clamped =
      std::clamp(percent, kStatusZoomMinimumPercent, kStatusZoomMaximumPercent);
  const float ratio = kStatusZoomMaximumPercent / kStatusZoomMinimumPercent;
  return 100.0f * std::log(clamped / kStatusZoomMinimumPercent) /
         std::log(ratio);
}

[[nodiscard]] inline std::optional<GalleryTab>
ParseGalleryTab(const std::string_view value) {
  if (value == "components") {
    return GalleryTab::Components;
  }
  if (value == "shell") {
    return GalleryTab::Shell;
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
  states[7].resize_start_height = kOperationTrayMaximumHeight;
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
