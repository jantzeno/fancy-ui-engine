#include "fancy_ui/components/progress_bar.hpp"

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

void ProgressBar(const ProgressBarSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float width = spec.size.x > 0.0f ? Scale(spec.size.x)
                                         : ImGui::GetContentRegionAvail().x;
  const float height = spec.size.y > 0.0f ? Scale(spec.size.y)
                                          : metrics.geometry.progress_height;
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
