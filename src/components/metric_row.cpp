#include "fancy_ui/components/metric_row.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>

namespace fancy_ui {

void MetricRow(const MetricRowSpec &spec) {
  ImGui::PushID(detail::Owned(spec.id).c_str());
  const bool expanded_layout =
      std::ranges::any_of(spec.metrics, [](const MetricValue &metric) {
        return metric.wide || metric.stacked;
      });
  const int columns =
      expanded_layout ? 2 : static_cast<int>(spec.metrics.size() + 1);
  if (ImGui::BeginTable("##metrics", columns,
                        ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(detail::Owned(spec.label).c_str());
    for (std::size_t index = 0; index < spec.metrics.size(); ++index) {
      const MetricValue &metric = spec.metrics[index];
      if (expanded_layout) {
        if (index > 0) {
          ImGui::TableNextRow();
        }
        ImGui::TableSetColumnIndex(1);
      } else {
        ImGui::TableNextColumn();
      }
      detail::DrawSecondaryText(metric.label);
      if (!metric.stacked) {
        ImGui::SameLine(0.0f, CurrentLayoutMetrics().spacing.space03);
      }
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
