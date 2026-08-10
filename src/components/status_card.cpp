#include "fancy_ui/components/status_card.hpp"

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

ColorRgba FromImVec4(const ImVec4 color) {
  return {.red = color.x, .green = color.y, .blue = color.z, .alpha = color.w};
}

std::string DefaultTitle(const SemanticStatus status) {
  switch (status) {
  case SemanticStatus::Information:
    return "INFORMATION";
  case SemanticStatus::Success:
    return "SUCCESS";
  case SemanticStatus::Warning:
    return "WARNING";
  case SemanticStatus::Failure:
    return "ERROR";
  case SemanticStatus::Busy:
    return "BUSY";
  case SemanticStatus::Preview:
    return "PREVIEW";
  case SemanticStatus::Neutral:
    return "STATUS";
  }
  return "STATUS";
}

} // namespace

void StatusCard(const StatusCardSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string title = spec.title.empty() ? DefaultTitle(spec.status)
                                               : detail::Owned(spec.title);
  const std::string message = detail::Owned(spec.message);
  const SemanticPalette &palette = CurrentPalette();
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ImGui::PushFont(nullptr, metrics.typography.body_font_height);
  const ImVec4 foreground = detail::StatusColor(spec.status);
  const ImVec4 background = detail::StatusBackground(spec.status);
  const float line_height = metrics.typography.body_font_height;
  const float copy_height = line_height * 2.0f;
  const float height =
      std::max(Scale(46.0f), copy_height + metrics.spacing.space02 * 2.0f);

  ImGui::PushID(id.c_str());
  ImGui::InvisibleButton("##status-card",
                         ImVec2(ImGui::GetContentRegionAvail().x, height),
                         ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(minimum, maximum, ImGui::GetColorU32(background));
  draw_list->AddRectFilled(minimum, ImVec2(minimum.x + Scale(3.0f), maximum.y),
                           ImGui::GetColorU32(foreground));

  float copy_x = minimum.x + metrics.spacing.space03;
  if (spec.icon) {
    const float icon_size = metrics.geometry.icon;
    const float icon_y = minimum.y + (height - icon_size) * 0.5f;
    spec.icon({.minimum = {.x = copy_x, .y = icon_y},
               .maximum = {.x = copy_x + icon_size, .y = icon_y + icon_size}},
              FromImVec4(foreground));
    copy_x += icon_size + metrics.spacing.space03;
  }
  const float copy_y = minimum.y + (height - copy_height) * 0.5f;
  const float copy_right = maximum.x - metrics.spacing.space03;
  const float copy_width = std::max(0.0f, copy_right - copy_x);
  const bool truncated = ImGui::CalcTextSize(message.c_str()).x > copy_width;
  const std::string visible_message =
      detail::EllipsizeText(message, copy_width);
  draw_list->PushClipRect(ImVec2(copy_x, minimum.y),
                          ImVec2(copy_right, maximum.y), true);
  ImFont *font = ImGui::GetFont();
  draw_list->AddText(font, line_height, ImVec2(copy_x, copy_y),
                     ImGui::GetColorU32(foreground), title.c_str());
  draw_list->AddText(font, line_height, ImVec2(copy_x, copy_y + line_height),
                     ImGui::GetColorU32(ToImVec4(palette.text_primary)),
                     visible_message.c_str());
  draw_list->PopClipRect();
  detail::DrawFocusRing(interaction);
  if (truncated && (interaction.hovered ||
                    (interaction.focused && ImGui::GetIO().NavVisible))) {
    detail::ShowTooltip(message);
  }
  ImGui::PopID();
  ImGui::PopFont();
}

} // namespace fancy_ui
