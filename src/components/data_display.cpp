#include "fancy_ui/components/data_display.hpp"

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

std::string Ellipsize(const std::string &text, const float width) {
  if (width <= 0.0f || ImGui::CalcTextSize(text.c_str()).x <= width) {
    return text;
  }
  constexpr std::string_view suffix = "...";
  const float suffix_width = ImGui::CalcTextSize(suffix.data()).x;
  std::size_t length = text.size();
  while (length > 0) {
    --length;
    while (length > 0 &&
           (static_cast<unsigned char>(text[length]) & 0xc0U) == 0x80U) {
      --length;
    }
    const std::string candidate = text.substr(0, length);
    if (ImGui::CalcTextSize(candidate.c_str()).x + suffix_width <= width) {
      return candidate + std::string(suffix);
    }
  }
  return width >= suffix_width ? std::string(suffix) : std::string{};
}

} // namespace

void StatusText(const StatusTextSpec &spec) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  const std::string label = detail::Owned(spec.label);
  ImGui::TextColored(detail::StatusColor(spec.status), "%s", label.c_str());
  ImGui::PopFont();
}

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
  const std::string visible_value = Ellipsize(complete_value, value_width);
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
