#include "fancy_ui/components/feedback.hpp"

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
  return {
      .red = color.x,
      .green = color.y,
      .blue = color.z,
      .alpha = color.w,
  };
}

std::string DefaultTitle(const SemanticStatus status) {
  switch (status) {
  case SemanticStatus::Information:
    return "INFO";
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
  const ImVec4 foreground = detail::StatusColor(spec.status);
  const ImVec4 background = detail::StatusBackground(spec.status);
  const float height = Scale(46.0f);

  ImGui::PushID(id.c_str());
  ImGui::InvisibleButton("##status-card",
                         ImVec2(ImGui::GetContentRegionAvail().x, height));
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(minimum, maximum, ImGui::GetColorU32(background));
  draw_list->AddRectFilled(minimum, ImVec2(minimum.x + Scale(3.0f), maximum.y),
                           ImGui::GetColorU32(foreground));

  float copy_x = minimum.x + Scale(8.0f);
  if (spec.icon) {
    const float icon_size = Scale(16.0f);
    const float icon_y = minimum.y + (height - icon_size) * 0.5f;
    spec.icon({.minimum = {.x = copy_x, .y = icon_y},
               .maximum = {.x = copy_x + icon_size, .y = icon_y + icon_size}},
              FromImVec4(foreground));
    copy_x += Scale(23.0f);
  }
  draw_list->PushClipRect(ImVec2(copy_x, minimum.y),
                          ImVec2(maximum.x - Scale(7.0f), maximum.y), true);
  ImFont *font = ImGui::GetFont();
  draw_list->AddText(font, Scale(13.0f),
                     ImVec2(copy_x, minimum.y + Scale(4.0f)),
                     ImGui::GetColorU32(foreground), title.c_str());
  draw_list->AddText(
      font, Scale(14.0f), ImVec2(copy_x, minimum.y + Scale(23.0f)),
      ImGui::GetColorU32(ToImVec4(palette.text_primary)), message.c_str());
  draw_list->PopClipRect();
  ImGui::PopID();
}

void Notification(const NotificationSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string title = detail::Owned(spec.title);
  const std::string message = detail::Owned(spec.message);
  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        detail::StatusBackground(spec.status));
  ImGui::PushStyleColor(ImGuiCol_Border, detail::StatusColor(spec.status));
  ImGui::PushID(id.c_str());
  if (ImGui::BeginChild("##notification", ImVec2(0.0f, 0.0f),
                        ImGuiChildFlags_Borders |
                            ImGuiChildFlags_AutoResizeY)) {
    if (spec.icon) {
      const ImVec2 cursor = ImGui::GetCursorScreenPos();
      const float icon_size = Scale(16.0f);
      spec.icon(
          {.minimum = {.x = cursor.x, .y = cursor.y},
           .maximum = {.x = cursor.x + icon_size, .y = cursor.y + icon_size}},
          FromImVec4(detail::StatusColor(spec.status)));
      ImGui::Dummy(ImVec2(icon_size, icon_size));
      ImGui::SameLine();
    }
    ImGui::TextColored(detail::StatusColor(spec.status), "%s", title.c_str());
    ImGui::TextWrapped("%s", message.c_str());
  }
  ImGui::EndChild();
  ImGui::PopID();
  ImGui::PopStyleColor(2);
}

void ProgressBar(const ProgressBarSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const float width = spec.size.x > 0.0f ? Scale(spec.size.x)
                                         : ImGui::GetContentRegionAvail().x;
  const float height = Scale(std::max(spec.size.y, 1.0f));
  const float progress = std::clamp(spec.value.value_or(0.46f), 0.0f, 1.0f);

  ImGui::PushID(id.c_str());
  ImGui::InvisibleButton("##progress", ImVec2(width, height));
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(
      minimum, maximum, ImGui::GetColorU32(ToImVec4(CurrentPalette().border)),
      height * 0.5f);
  const ImVec2 filled_max(minimum.x + (maximum.x - minimum.x) * progress,
                          maximum.y);
  draw_list->AddRectFilled(minimum, filled_max,
                           ImGui::GetColorU32(detail::StatusColor(spec.status)),
                           height * 0.5f);
  if ((ImGui::IsItemHovered() ||
       (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible)) &&
      !spec.label.empty()) {
    detail::ShowTooltip(spec.label);
  }
  ImGui::PopID();
}

} // namespace fancy_ui
