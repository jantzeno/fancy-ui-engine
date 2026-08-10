#include "fancy_ui/components/value_display.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

InteractionResult ValueDisplay(const ValueDisplaySpec &spec) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ImGui::PushFont(nullptr, metrics.typography.body_font_height);
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  const std::string complete_value =
      spec.mixed ? std::string("Mixed") : detail::Owned(spec.value);
  const float height = metrics.geometry.control_height;

  ImGui::PushID(id.c_str());
  ImGui::InvisibleButton("##value-display",
                         ImVec2(ImGui::GetContentRegionAvail().x, height),
                         ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(
      minimum, maximum,
      ImGui::GetColorU32(ToImVec4(CurrentPalette().surface_raised)),
      Scale(3.0f));
  draw_list->AddRect(
      minimum, maximum,
      ImGui::GetColorU32(ToImVec4(CurrentPalette().border_strong)), Scale(3.0f),
      ImDrawFlags_RoundCornersAll, Scale(1.0f));
  const float label_width =
      std::max(Scale(std::max(spec.label_width, 0.0f)),
               ImGui::CalcTextSize(label.c_str()).x + Scale(8.0f));
  const float text_y = minimum.y + (height - ImGui::GetTextLineHeight()) * 0.5f;
  draw_list->AddText(
      ImVec2(minimum.x + Scale(8.0f), text_y),
      ImGui::GetColorU32(ToImVec4(CurrentPalette().text_secondary)),
      label.c_str());
  const float value_x = minimum.x + Scale(8.0f) + label_width;
  const float value_width = std::max(0.0f, maximum.x - value_x - Scale(8.0f));
  const std::string visible_value =
      detail::EllipsizeText(complete_value, value_width);
  draw_list->PushClipRect(ImVec2(value_x, minimum.y),
                          ImVec2(maximum.x - Scale(8.0f), maximum.y), true);
  draw_list->AddText(
      ImVec2(value_x, text_y),
      ImGui::GetColorU32(ToImVec4(spec.mixed ? CurrentPalette().text_secondary
                                             : CurrentPalette().text_primary)),
      visible_value.c_str());
  draw_list->PopClipRect();
  detail::DrawFocusRing(interaction);
  if (interaction.hovered ||
      (interaction.focused && ImGui::GetIO().NavVisible)) {
    const std::string tooltip =
        spec.tooltip.empty() ? complete_value : detail::Owned(spec.tooltip);
    if (!tooltip.empty()) {
      detail::ShowTooltip(tooltip);
    }
  }
  ImGui::PopID();
  ImGui::PopFont();
  return interaction;
}

} // namespace fancy_ui
