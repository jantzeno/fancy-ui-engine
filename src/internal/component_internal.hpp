#pragma once

#include "fancy_ui/component_types.hpp"

#include <imgui.h>

#include <string>
#include <string_view>

namespace fancy_ui::detail {

[[nodiscard]] std::string Owned(std::string_view value);
void BeginAvailability(const Availability &availability);
void EndAvailability(const Availability &availability,
                     std::string_view tooltip);
[[nodiscard]] InteractionResult CaptureInteraction();
[[nodiscard]] ImVec4 StatusColor(SemanticStatus status);
[[nodiscard]] ImVec4 StatusBackground(SemanticStatus status);

} // namespace fancy_ui::detail
