#include "fancy_ui/components/metric_row.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

namespace fancy_ui {

void MetricRow(const MetricRowSpec &spec) {
  ImGui::PushID(detail::Owned(spec.id).c_str());
  if (ImGui::BeginTable("##metrics", static_cast<int>(spec.metrics.size() + 1),
                        ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(detail::Owned(spec.label).c_str());
    for (const MetricValue &metric : spec.metrics) {
      ImGui::TableNextColumn();
      detail::DrawSecondaryText(metric.label);
      ImGui::TextUnformatted(detail::Owned(metric.value).c_str());
    }
    ImGui::EndTable();
  }
  const float consumed = ImGui::GetItemRectSize().y;
  if (consumed < Scale(spec.minimum_height)) {
    ImGui::Dummy(ImVec2(0.0f, Scale(spec.minimum_height) - consumed));
  }
  ImGui::PopID();
}

} // namespace fancy_ui
