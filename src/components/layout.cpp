#include "fancy_ui/components/layout.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace fancy_ui {

SectionResult BeginSection(const SectionSpec &spec) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const std::string id = detail::Owned(spec.id);
  const std::string heading = detail::Owned(spec.heading);
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
    ImGui::Indent(metrics.spacing.space04);
  }
  return {.visible = true, .open = open};
}

void EndSection(const SectionResult &result) {
  if (result.open) {
    ImGui::Unindent(CurrentLayoutMetrics().spacing.space04);
  }
  ImGui::PopID();
}

} // namespace fancy_ui
