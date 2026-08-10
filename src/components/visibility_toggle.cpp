#include "fancy_ui/components/visibility_toggle.hpp"

#include "fancy_ui/components/checkbox.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <string>

namespace fancy_ui {

VisibilityToggleResult VisibilityToggle(const VisibilityToggleSpec &spec) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  ImGui::PushID(detail::Owned(spec.id).c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  const std::string state_label = spec.state == ToggleState::On    ? "Visible"
                                  : spec.state == ToggleState::Off ? "Hidden"
                                                                   : "Mixed";
  const CheckboxResult checkbox = Checkbox({
      .id = "value",
      .label = state_label,
      .tooltip = spec.tooltip,
      .state = spec.state,
      .on_icon = spec.visible_icon,
      .off_icon = spec.hidden_icon,
      .show_checkbox = true,
      .availability = spec.availability,
  });
  detail::EndFieldLayout(layout, {});
  ImGui::PopID();

  VisibilityToggleResult result;
  static_cast<InteractionResult &>(result) = checkbox;
  result.changed = checkbox.changed;
  result.state = checkbox.state;
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
