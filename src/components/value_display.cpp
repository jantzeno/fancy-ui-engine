#include "fancy_ui/components/value_display.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

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
  const std::string complete_value =
      spec.mixed ? std::string("Mixed") : detail::Owned(spec.value);
  const float height = metrics.geometry.control_height;

  ImGui::PushID(id.c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, height));
  const InteractionResult interaction{.hovered = ImGui::IsItemHovered()};
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const float text_y = minimum.y + (height - ImGui::GetTextLineHeight()) * 0.5f;
  const float value_width = maximum.x - minimum.x;
  const std::string visible_value =
      detail::EllipsizeText(complete_value, value_width);
  draw_list->PushClipRect(minimum, maximum, true);
  draw_list->AddText(
      ImVec2(minimum.x, text_y),
      ImGui::GetColorU32(ToImVec4(spec.mixed ? CurrentPalette().text_secondary
                                             : CurrentPalette().text_primary)),
      visible_value.c_str());
  draw_list->PopClipRect();
  if (interaction.hovered) {
    const std::string tooltip =
        spec.tooltip.empty() ? complete_value : detail::Owned(spec.tooltip);
    if (!tooltip.empty()) {
      detail::ShowTooltip(tooltip);
    }
  }
  detail::EndFieldLayout(layout, {});
  ImGui::PopID();
  ImGui::PopFont();
  return interaction;
}

} // namespace fancy_ui
