#include "fancy_ui/components/status_text.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <string>

namespace fancy_ui {

void StatusText(const StatusTextSpec &spec) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  const std::string label = detail::Owned(spec.label);
  ImGui::TextColored(detail::StatusColor(spec.status), "%s", label.c_str());
  ImGui::PopFont();
}

} // namespace fancy_ui
