#include "fancy_ui/components/section.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace fancy_ui {

SectionResult BeginSection(const SectionSpec &spec) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const LayoutMetrics logical = LogicalLayoutMetrics();
  const std::string id = detail::Owned(spec.id);
  const std::string heading = detail::Owned(spec.heading);
  const std::string summary = detail::Owned(spec.summary);
  if (spec.separated) {
    const ImVec2 minimum = ImGui::GetCursorScreenPos();
    const float divider_y = minimum.y + metrics.spacing.space04;
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(minimum.x, divider_y),
        ImVec2(minimum.x + ImGui::GetContentRegionAvail().x, divider_y),
        ImGui::GetColorU32(ImGuiCol_Border), metrics.geometry.border);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                         metrics.spacing.space04 * 2.0f +
                         metrics.geometry.border);
  }
  ImGui::PushID(id.c_str());
  ImGui::SetNextItemOpen(spec.open, ImGuiCond_Always);
  const float vertical_padding =
      std::max(0.0f, std::floor((metrics.inspector.section_header_height -
                                 ImGui::GetFontSize()) *
                                0.5f));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(metrics.spacing.space03, vertical_padding));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  const bool open = ImGui::CollapsingHeader(heading.c_str());
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
  const ImVec2 header_minimum = ImGui::GetItemRectMin();
  const ImVec2 header_maximum = ImGui::GetItemRectMax();
  const ImVec2 cursor_after_header = ImGui::GetCursorScreenPos();
  if (spec.focused) {
    const ColorRgba focus = CurrentPalette().focus;
    ImGui::GetWindowDrawList()->AddRect(
        header_minimum, header_maximum,
        ImGui::ColorConvertFloat4ToU32(
            ImVec4(focus.red, focus.green, focus.blue, focus.alpha)));
  }
  if (!summary.empty()) {
    const ImVec2 summary_size = ImGui::CalcTextSize(summary.c_str());
    const float action_width =
        spec.header_action.has_value() ? metrics.geometry.compact_target : 0.0f;
    const float summary_x =
        std::max(header_minimum.x + metrics.spacing.space08,
                 header_maximum.x - metrics.spacing.space04 - action_width -
                     summary_size.x);
    const ColorRgba text = CurrentPalette().text_secondary;
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(summary_x, std::floor((header_minimum.y + header_maximum.y -
                                      summary_size.y) *
                                     0.5f)),
        ImGui::ColorConvertFloat4ToU32(
            ImVec4(text.red, text.green, text.blue, text.alpha)),
        summary.c_str());
  }
  bool header_action_activated = false;
  if (spec.header_action.has_value()) {
    ButtonSpec action = *spec.header_action;
    action.size = {logical.geometry.compact_target,
                   logical.geometry.compact_target};
    ImGui::SetCursorScreenPos(
        ImVec2(header_maximum.x - metrics.spacing.space02 -
                   metrics.geometry.compact_target,
               header_minimum.y + (header_maximum.y - header_minimum.y -
                                   metrics.geometry.compact_target) *
                                      0.5f));
    header_action_activated = Button(action).activated;
    ImGui::SetCursorScreenPos(cursor_after_header);
  }
  if (open) {
    ImGui::Dummy(ImVec2(0.0f, metrics.spacing.space02));
  }
  return {
      .open = open,
      .open_changed = open != spec.open,
      .header_action_activated = header_action_activated,
  };
}

void EndSection(const SectionResult &) { ImGui::PopID(); }

} // namespace fancy_ui
