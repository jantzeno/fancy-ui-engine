#include "fancy_ui/components/checked_multiselect.hpp"

#include "fancy_ui/components/checkbox.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>

namespace fancy_ui {

CheckedMultiselectResult
CheckedMultiselect(const CheckedMultiselectSpec &spec) {
  CheckedMultiselectResult result;
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  ImGui::PushID(detail::Owned(spec.id).c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::PushFieldControlState(spec.availability, {});
  detail::BeginAvailability(spec.availability);

  const bool activated =
      ImGui::Button(detail::Owned(spec.summary).c_str(),
                    ImVec2(ImGui::GetContentRegionAvail().x,
                           CurrentLayoutMetrics().geometry.control_height));
  const InteractionResult interaction = detail::CaptureInteraction();
  static_cast<InteractionResult &>(result) = interaction;
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const float arrow = Scale(4.0f);
  const ImVec2 center(maximum.x - Scale(12.0f), (minimum.y + maximum.y) * 0.5f);
  ImGui::GetWindowDrawList()->AddTriangleFilled(
      ImVec2(center.x - arrow, center.y - arrow * 0.5f),
      ImVec2(center.x + arrow, center.y - arrow * 0.5f),
      ImVec2(center.x, center.y + arrow * 0.5f),
      ImGui::GetColorU32(ImVec4(CurrentPalette().text_secondary.red,
                                CurrentPalette().text_secondary.green,
                                CurrentPalette().text_secondary.blue,
                                CurrentPalette().text_secondary.alpha)));
  detail::DrawFocusRing(interaction);
  if (activated || spec.request_open) {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const float estimated_height =
        static_cast<float>(spec.options.size()) *
            CurrentLayoutMetrics().geometry.compact_target +
        CurrentLayoutMetrics().spacing.space02 * 2.0f;
    const float popup_y =
        maximum.y + estimated_height <=
                viewport->WorkPos.y + viewport->WorkSize.y
            ? maximum.y
            : std::max(viewport->WorkPos.y, minimum.y - estimated_height);
    ImGui::SetNextWindowPos(ImVec2(minimum.x, popup_y), ImGuiCond_Appearing);
    ImGui::OpenPopup("##options");
  }
  detail::PushMenuPopupStyle();
  if (ImGui::BeginPopup("##options")) {
    result.popup_open = true;
    for (const CheckedMultiselectOption &option : spec.options) {
      const CheckboxResult checkbox = Checkbox({
          .id = option.id,
          .label = option.label,
          .state = option.state,
          .availability = option.availability,
      });
      if (checkbox.changed) {
        result.changed = true;
        result.option_id = detail::Owned(option.id);
        result.state = checkbox.state;
      }
    }
    ImGui::EndPopup();
  }
  detail::PopMenuPopupStyle();
  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::PopFieldControlState(spec.availability, {});
  detail::EndFieldLayout(layout, {});
  ImGui::PopID();
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
