#include "fancy_ui/components/data_display.hpp"

#include "internal/component_internal.hpp"

#include <imgui.h>

namespace fancy_ui {

void StatusText(const StatusTextSpec &spec) {
  const std::string label = detail::Owned(spec.label);
  ImGui::TextColored(detail::StatusColor(spec.status), "%s", label.c_str());
}

} // namespace fancy_ui
