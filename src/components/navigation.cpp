#include "fancy_ui/components/navigation.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

namespace fancy_ui {

NavigationItemResult NavigationItem(const NavigationItemSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string label = detail::Owned(spec.label);
  ImGui::PushID(id.c_str());
  detail::BeginAvailability(spec.availability);
  const bool activated =
      ImGui::Selectable(label.c_str(), spec.selected, 0, ImVec2(0.0f, 32.0f));
  const InteractionResult interaction = detail::CaptureInteraction();
  detail::EndAvailability(spec.availability, spec.tooltip);
  ImGui::PopID();

  NavigationItemResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.activated = activated;
  return result;
}

} // namespace fancy_ui
