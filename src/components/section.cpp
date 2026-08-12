#include "fancy_ui/components/section.hpp"

#include "fancy_ui/components/disclosure_row.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace fancy_ui {

SectionResult BeginSection(const SectionSpec &spec) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const LayoutMetrics logical = LogicalLayoutMetrics();
  const std::string id = detail::Owned(spec.id);
  float action_width = 0.0f;
  if (spec.header_action.has_value()) {
    action_width = std::max(
        metrics.geometry.compact_target,
        ImGui::CalcTextSize(detail::Owned(spec.header_action->label).c_str())
                .x +
            metrics.spacing.space04 * 2.0f);
  }
  ImGui::PushID(id.c_str());
  const DisclosureRowResult disclosure = DisclosureRow({
      .id = "header",
      .label = spec.heading,
      .metadata = spec.summary,
      .variant = DisclosureRowVariant::PanelHeader,
      .expandable = true,
      .expanded = spec.open,
      .font = spec.heading_font,
      .reserved_trailing_width =
          (action_width > 0.0f ? action_width + metrics.spacing.space02
                               : 0.0f) /
          CurrentUiScale(),
  });
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
  bool header_action_activated = false;
  if (spec.header_action.has_value()) {
    ButtonSpec action = *spec.header_action;
    action.size = {action_width / CurrentUiScale(),
                   logical.geometry.compact_target};
    ImGui::SetCursorScreenPos(
        ImVec2(header_maximum.x - action_width,
               header_minimum.y + (header_maximum.y - header_minimum.y -
                                   metrics.geometry.compact_target) *
                                      0.5f));
    header_action_activated = Button(action).activated;
    ImGui::SetCursorScreenPos(cursor_after_header);
  }
  bool open = disclosure.expanded;
  if (disclosure.activated) {
    open = !open;
  }
  const ImVec2 item_spacing = ImGui::GetStyle().ItemSpacing;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(item_spacing.x, metrics.spacing.space02));
  return {
      .open = open,
      .open_changed = open != spec.open,
      .header_action_activated = header_action_activated,
  };
}

void EndSection(const SectionResult &) {
  ImGui::PopStyleVar();
  ImGui::PopID();
}

} // namespace fancy_ui
