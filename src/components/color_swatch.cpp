#include "fancy_ui/components/color_swatch.hpp"

#include "fancy_ui/components/color_picker_popup.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

ColorSwatchResult ColorSwatch(const ColorSwatchSpec &spec,
                              ColorPickerState &state) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  ImGui::PushID(detail::Owned(spec.id).c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::BeginAvailability(spec.availability);
  const ImVec2 size(Scale(32.0f), Scale(32.0f));
  const bool activated =
      ImGui::InvisibleButton("##swatch", size, ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const SemanticPalette &palette = CurrentPalette();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(minimum, maximum,
                           ImGui::GetColorU32(ToImVec4(palette.surface_raised)),
                           Scale(3.0f));
  draw_list->AddRect(minimum, maximum,
                     ImGui::GetColorU32(ToImVec4(palette.border_strong)),
                     Scale(3.0f), ImDrawFlags_RoundCornersAll, Scale(1.0f));
  const ImVec2 inset(Scale(4.0f), Scale(4.0f));
  const ImVec2 inner_min(minimum.x + inset.x, minimum.y + inset.y);
  const ImVec2 inner_max(maximum.x - inset.x, maximum.y - inset.y);
  const std::size_t count = std::max<std::size_t>(spec.colors.size(), 1);
  for (std::size_t index = 0; index < count; ++index) {
    const ColorRgba color =
        spec.colors.empty() ? palette.surface_muted : spec.colors[index];
    const float left = inner_min.x + (inner_max.x - inner_min.x) *
                                         static_cast<float>(index) /
                                         static_cast<float>(count);
    const float right = inner_min.x + (inner_max.x - inner_min.x) *
                                          static_cast<float>(index + 1) /
                                          static_cast<float>(count);
    draw_list->AddRectFilled(ImVec2(left, inner_min.y),
                             ImVec2(right, inner_max.y),
                             ImGui::GetColorU32(ToImVec4(color)));
  }
  detail::DrawFocusRing(interaction, true);
  if (state.restore_focus) {
    ImGui::SetKeyboardFocusHere(-1);
    state.restore_focus = false;
  }
  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::EndFieldLayout(layout, {});

  const bool request_open =
      activated && spec.availability.enabled && !spec.availability.busy;
  const ColorPickerPopupResult picker = ColorPickerPopup(
      {
          .id = "picker",
          .title = spec.picker_title,
          .value = spec.value,
          .request_open = request_open,
          .show_alpha = spec.show_alpha,
          .layout = spec.picker_layout,
      },
      state);
  ImGui::PopID();

  ColorSwatchResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = request_open;
  result.changed = picker.changed;
  result.committed = picker.committed;
  result.cancelled = picker.cancelled;
  result.picker_open = picker.picker_open;
  result.value = picker.value;
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
