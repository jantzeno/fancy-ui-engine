#include "fancy_ui/components/notification.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <string>

namespace fancy_ui {

namespace {

ColorRgba FromImVec4(const ImVec4 color) {
  return {.red = color.x, .green = color.y, .blue = color.z, .alpha = color.w};
}

} // namespace

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
    if (spec.icon) {
      const ImVec2 cursor = ImGui::GetCursorScreenPos();
      const float icon_size = Scale(16.0f);
      spec.icon(
          {.minimum = {.x = cursor.x, .y = cursor.y},
           .maximum = {.x = cursor.x + icon_size, .y = cursor.y + icon_size}},
          FromImVec4(detail::StatusColor(spec.status)));
      ImGui::Dummy(ImVec2(icon_size, icon_size));
      ImGui::SameLine();
    }
    ImGui::TextColored(detail::StatusColor(spec.status), "%s", title.c_str());
    ImGui::TextWrapped("%s", message.c_str());
  }
  ImGui::EndChild();
  ImGui::PopID();
  ImGui::PopStyleColor(2);
}

} // namespace fancy_ui
