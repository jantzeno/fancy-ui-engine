#include "fancy_ui/components/section.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace fancy_ui {

SectionResult BeginSection(const SectionSpec &spec) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const std::string id = detail::Owned(spec.id);
  const std::string heading = detail::Owned(spec.heading);
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
  ImGui::SetNextItemOpen(spec.initially_open, ImGuiCond_Once);
  const float vertical_padding =
      std::max(0.0f, std::floor((metrics.inspector.section_header_height -
                                 ImGui::GetFontSize()) *
                                0.5f));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(metrics.spacing.space03, vertical_padding));
  const bool open = ImGui::CollapsingHeader(heading.c_str());
  ImGui::PopStyleVar();
  if (open) {
    ImGui::Dummy(ImVec2(0.0f, metrics.spacing.space02));
    ImGui::Indent(metrics.spacing.space04);
  }
  return {.open = open};
}

void EndSection(const SectionResult &result) {
  if (result.open) {
    ImGui::Unindent(CurrentLayoutMetrics().spacing.space04);
  }
  ImGui::PopID();
}

} // namespace fancy_ui
