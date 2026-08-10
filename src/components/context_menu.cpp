#include "fancy_ui/components/context_menu.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace fancy_ui {

namespace {

bool DrawContextMenuItems(const std::span<const ContextMenuItemSpec> items,
                          ContextMenuResult &result) {
  for (const ContextMenuItemSpec &item : items) {
    if (item.kind == ContextMenuItemKind::Separator) {
      ImGui::Separator();
      continue;
    }
    ImGui::PushID(detail::Owned(item.id).c_str());
    const bool enabled = item.availability.enabled && !item.availability.busy;
    if (item.kind == ContextMenuItemKind::Submenu) {
      if (ImGui::BeginMenu(detail::Owned(item.label).c_str(), enabled)) {
        if (DrawContextMenuItems(item.children, result)) {
          ImGui::EndMenu();
          ImGui::PopID();
          return true;
        }
        ImGui::EndMenu();
      }
    } else if (ImGui::MenuItem(detail::Owned(item.label).c_str(),
                               detail::Owned(item.shortcut).c_str(),
                               item.selected, enabled)) {
      result.activated_id = detail::Owned(item.id);
      ImGui::PopID();
      return true;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      const std::string_view tooltip =
          enabled ? item.tooltip : item.availability.reason;
      if (!tooltip.empty()) {
        detail::ShowTooltip(tooltip);
      }
    }
    ImGui::PopID();
  }
  return false;
}

} // namespace

ContextMenuResult ContextMenu(const ContextMenuSpec &spec,
                              ContextMenuState &state) {
  ContextMenuResult result;
  ImGui::PushID(detail::Owned(spec.id).c_str());
  if (state.restore_focus) {
    ImGui::SetKeyboardFocusHere(-1);
    state.restore_focus = false;
  }
  if (spec.request_open) {
    if (spec.anchor.has_value()) {
      const ImGuiViewport *viewport = ImGui::GetMainViewport();
      const LayoutMetrics metrics = CurrentLayoutMetrics();
      const float estimated_height = static_cast<float>(spec.items.size()) *
                                         metrics.geometry.compact_target +
                                     metrics.spacing.space06;
      const ImVec2 maximum(
          viewport->WorkPos.x + viewport->WorkSize.x - metrics.menu.popup_width,
          viewport->WorkPos.y + viewport->WorkSize.y - estimated_height);
      const ImVec2 clamped_maximum(std::max(viewport->WorkPos.x, maximum.x),
                                   std::max(viewport->WorkPos.y, maximum.y));
      ImGui::SetNextWindowPos(ImVec2(
          std::clamp(spec.anchor->x, viewport->WorkPos.x, clamped_maximum.x),
          std::clamp(spec.anchor->y, viewport->WorkPos.y, clamped_maximum.y)));
    }
    ImGui::OpenPopup("##context-menu");
    result.opened = true;
  }
  bool open = false;
  if (ImGui::BeginPopup("##context-menu")) {
    open = true;
    state.open = true;
    if (DrawContextMenuItems(spec.items, result)) {
      ImGui::CloseCurrentPopup();
      open = false;
    }
    ImGui::EndPopup();
  }
  if (state.open && !open) {
    result.closed = true;
    state.open = false;
    state.restore_focus = true;
  }
  result.menu_open = state.open;
  ImGui::PopID();
  return result;
}

} // namespace fancy_ui
