#include "fancy_ui/components/explorer_search.hpp"

#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <limits>
#include <vector>

namespace fancy_ui {

ExplorerSearchResult ExplorerSearch(const ExplorerSearchSpec &spec) {
  const std::size_t capacity = std::clamp<std::size_t>(spec.capacity, 2, 4096);
  std::vector<char> buffer(capacity, '\0');
  std::copy_n(spec.query.data(), std::min(spec.query.size(), capacity - 1),
              buffer.data());
  ImGui::PushID(detail::Owned(spec.id).c_str());
  ImGui::SetNextItemWidth(-std::numeric_limits<float>::min());
  detail::BeginAvailability(spec.availability);
  const bool changed = ImGui::InputTextWithHint(
      "##search", detail::Owned(spec.placeholder).c_str(), buffer.data(),
      buffer.size());
  const InteractionResult interaction = detail::CaptureInteraction();
  const bool cancelled = detail::CancelledThisFrame(interaction);
  const bool committed = ImGui::IsItemDeactivatedAfterEdit() && !cancelled;
  detail::DrawFocusRing(interaction);
  detail::EndAvailability(spec.availability, {});
  ImGui::PopID();

  ExplorerSearchResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = changed && !cancelled;
  result.committed = committed;
  result.cancelled = cancelled;
  result.query =
      cancelled ? std::string(spec.query) : std::string(buffer.data());
  return result;
}

} // namespace fancy_ui
