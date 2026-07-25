#include "fancy_ui/components/layout.hpp"

#include "internal/component_internal.hpp"

#include <imgui.h>

namespace fancy_ui {

SectionResult BeginSection(const SectionSpec &spec) {
  const std::string id = detail::Owned(spec.id);
  const std::string heading = detail::Owned(spec.heading);
  ImGui::PushID(id.c_str());
  ImGui::SetNextItemOpen(spec.initially_open, ImGuiCond_Once);
  const bool open = ImGui::CollapsingHeader(heading.c_str());
  if (open) {
    ImGui::Indent(16.0f);
  }
  return {.visible = true, .open = open};
}

void EndSection(const SectionResult &result) {
  if (result.open) {
    ImGui::Unindent(16.0f);
  }
  ImGui::PopID();
}

} // namespace fancy_ui
