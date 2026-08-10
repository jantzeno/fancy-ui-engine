#include "fancy_ui/components/status_zoom_popover.hpp"

#include "fancy_ui/components/button.hpp"
#include "fancy_ui/components/slider.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <format>

namespace fancy_ui {

float StatusZoomPercentFromSliderPosition(const float position) {
  const float normalized = std::clamp(position, 0.0f, 100.0f) / 100.0f;
  const float ratio = kStatusZoomMaximumPercent / kStatusZoomMinimumPercent;
  return std::round(kStatusZoomMinimumPercent * std::pow(ratio, normalized));
}

float StatusZoomSliderPositionFromPercent(const float percent) {
  const float clamped =
      std::clamp(percent, kStatusZoomMinimumPercent, kStatusZoomMaximumPercent);
  const float ratio = kStatusZoomMaximumPercent / kStatusZoomMinimumPercent;
  return 100.0f * std::log(clamped / kStatusZoomMinimumPercent) /
         std::log(ratio);
}

StatusZoomPopoverResult StatusZoomPopover(const StatusZoomPopoverSpec &spec,
                                          StatusZoomPopoverState &state) {
  StatusZoomPopoverResult result;
  result.percent = std::clamp(spec.percent, kStatusZoomMinimumPercent,
                              kStatusZoomMaximumPercent);
  ImGui::PushID(detail::Owned(spec.id).c_str());
  if (state.restore_focus) {
    ImGui::SetKeyboardFocusHere();
    state.restore_focus = false;
  }
  const ButtonResult trigger = Button({
      .id = "trigger",
      .label = std::format("{:.0f}%", result.percent),
      .tooltip = "Canvas zoom",
      .variant = ButtonVariant::Tertiary,
      .size = {.x = 72.0f, .y = 24.0f},
  });
  static_cast<InteractionResult &>(result) = trigger;
  if (trigger.activated || spec.request_open) {
    ImGui::OpenPopup("##zoom-popover");
    result.opened = true;
  }
  bool open = false;
  if (ImGui::BeginPopup("##zoom-popover")) {
    open = true;
    state.open = true;
    const auto command = [&result](const StatusZoomCommand value) {
      result.command = value;
      result.closed = true;
      ImGui::CloseCurrentPopup();
    };
    if (Button({.id = "actual", .label = "Zoom 100%"}).activated) {
      command(StatusZoomCommand::ActualSize);
    }
    if (Button({.id = "fit", .label = "Zoom to Fit"}).activated) {
      command(StatusZoomCommand::Fit);
    }
    if (Button({
                   .id = "selection",
                   .label = "Zoom to Selection",
                   .availability = spec.selection_availability,
               })
            .activated) {
      command(StatusZoomCommand::Selection);
    }
    float slider_position = StatusZoomSliderPositionFromPercent(result.percent);
    const SliderResult slider = Slider({
        .id = "slider",
        .label = "Zoom",
        .tooltip = "10–1600%",
        .unit = "%",
        .value = slider_position,
        .minimum = 0.0f,
        .maximum = 100.0f,
        .format = "%.0f",
    });
    if (slider.changed) {
      result.changed = true;
      result.percent = StatusZoomPercentFromSliderPosition(slider.value);
    }
    result.committed = slider.committed;
    ImGui::EndPopup();
  }
  if (result.closed) {
    open = false;
  }
  if (state.open && !open) {
    result.closed = true;
    state.open = false;
    state.restore_focus = true;
  }
  result.popup_open = state.open;
  ImGui::PopID();
  return result;
}

} // namespace fancy_ui
