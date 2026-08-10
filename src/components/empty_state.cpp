#include "fancy_ui/components/empty_state.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

void DrawDashedLine(ImDrawList *draw_list, const ImVec2 start, const ImVec2 end,
                    const ImU32 color, const float thickness) {
  const float length =
      std::max(std::abs(end.x - start.x), std::abs(end.y - start.y));
  const float segment = Scale(4.0f);
  const float gap = Scale(3.0f);
  const ImVec2 direction = length > 0.0f ? ImVec2((end.x - start.x) / length,
                                                  (end.y - start.y) / length)
                                         : ImVec2();
  for (float offset = 0.0f; offset < length; offset += segment + gap) {
    const float finish = std::min(offset + segment, length);
    draw_list->AddLine(
        ImVec2(start.x + direction.x * offset, start.y + direction.y * offset),
        ImVec2(start.x + direction.x * finish, start.y + direction.y * finish),
        color, thickness);
  }
}

void DrawDashedRect(ImDrawList *draw_list, const ImVec2 minimum,
                    const ImVec2 maximum, const ImU32 color) {
  const float thickness = Scale(1.0f);
  DrawDashedLine(draw_list, minimum, ImVec2(maximum.x, minimum.y), color,
                 thickness);
  DrawDashedLine(draw_list, ImVec2(maximum.x, minimum.y), maximum, color,
                 thickness);
  DrawDashedLine(draw_list, maximum, ImVec2(minimum.x, maximum.y), color,
                 thickness);
  DrawDashedLine(draw_list, ImVec2(minimum.x, maximum.y), minimum, color,
                 thickness);
}

} // namespace

void EmptyState(const EmptyStateSpec &spec) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  const std::string id = detail::Owned(spec.id);
  const std::string title = detail::Owned(spec.title);
  const std::string message = detail::Owned(spec.message);
  const float height = Scale(std::max(spec.minimum_height, 44.0f));

  ImGui::PushID(id.c_str());
  ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, height));
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  DrawDashedRect(draw_list, minimum, maximum,
                 ImGui::GetColorU32(ToImVec4(CurrentPalette().border)));

  const float icon_space = spec.icon ? Scale(22.0f) : 0.0f;
  const float message_space = message.empty() ? 0.0f : Scale(16.0f);
  const float content_height =
      icon_space + ImGui::GetTextLineHeight() + message_space;
  float content_y = minimum.y + (height - content_height) * 0.5f;
  if (spec.icon) {
    const float icon_size = Scale(16.0f);
    const float icon_x = minimum.x + (maximum.x - minimum.x - icon_size) * 0.5f;
    spec.icon(
        {.minimum = {.x = icon_x, .y = content_y},
         .maximum = {.x = icon_x + icon_size, .y = content_y + icon_size}},
        CurrentPalette().text_secondary);
    content_y += icon_space;
  }
  const ImVec2 title_size = ImGui::CalcTextSize(title.c_str());
  draw_list->AddText(
      ImVec2(minimum.x + (maximum.x - minimum.x - title_size.x) * 0.5f,
             content_y),
      ImGui::GetColorU32(ToImVec4(CurrentPalette().text_secondary)),
      title.c_str());
  if (!message.empty()) {
    const ImVec2 message_size = ImGui::CalcTextSize(message.c_str());
    draw_list->AddText(
        ImVec2(minimum.x + (maximum.x - minimum.x - message_size.x) * 0.5f,
               content_y + Scale(16.0f)),
        ImGui::GetColorU32(ToImVec4(CurrentPalette().text_disabled)),
        message.c_str());
  }
  ImGui::PopID();
  ImGui::PopFont();
}

} // namespace fancy_ui
