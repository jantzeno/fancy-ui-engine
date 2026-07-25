#include "fancy_ui/components/feedback.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

namespace fancy_ui {

void Notification(const NotificationSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string title = detail::Owned(spec.title);
  const std::string message = detail::Owned(spec.message);
  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        detail::StatusBackground(spec.status));
  ImGui::PushStyleColor(ImGuiCol_Border, detail::StatusColor(spec.status));
  ImGui::PushID(id.c_str());
  if (ImGui::BeginChild("##notification", ImVec2(0.0f, 0.0f),
                        ImGuiChildFlags_Borders |
                            ImGuiChildFlags_AutoResizeY)) {
    ImGui::TextColored(detail::StatusColor(spec.status), "%s", title.c_str());
    ImGui::TextWrapped("%s", message.c_str());
  }
  ImGui::EndChild();
  ImGui::PopID();
  ImGui::PopStyleColor(2);
}

} // namespace fancy_ui
