#include "fancy_ui/components/tab_set.hpp"

#include "fancy_ui/components/segmented_control.hpp"

#include "internal/component_internal.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace fancy_ui {

namespace {

bool Available(const ChoiceSpec &choice) {
  return choice.availability.enabled && !choice.availability.busy;
}

std::optional<std::size_t>
AdjacentAvailable(const std::span<const ChoiceSpec> tabs,
                  const std::size_t current, const int direction) {
  for (std::size_t tries = 0; tries < tabs.size(); ++tries) {
    const std::size_t candidate = static_cast<std::size_t>(
        (static_cast<int>(current) + direction * static_cast<int>(tries + 1) +
         static_cast<int>(tabs.size()) * static_cast<int>(tabs.size())) %
        static_cast<int>(tabs.size()));
    if (Available(tabs[candidate])) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::optional<std::size_t>
BoundaryAvailable(const std::span<const ChoiceSpec> tabs, const bool last) {
  for (std::size_t offset = 0; offset < tabs.size(); ++offset) {
    const std::size_t index = last ? tabs.size() - offset - 1 : offset;
    if (Available(tabs[index])) {
      return index;
    }
  }
  return std::nullopt;
}

} // namespace

TabSetResult TabSet(const TabSetSpec &spec) {
  TabSetResult result;
  if (spec.tabs.empty()) {
    return result;
  }

  const std::size_t clamped =
      std::min(spec.selected_index, spec.tabs.size() - std::size_t{1});
  const std::optional<std::size_t> first = BoundaryAvailable(spec.tabs, false);
  const std::size_t selected =
      Available(spec.tabs[clamped]) || !first.has_value() ? clamped : *first;
  result.selected_index = selected;

  if (spec.width > 0.0f) {
    const SegmentedControlResult segments = SegmentedControl({
        .id = spec.id,
        .choices = spec.tabs,
        .selected_index = selected,
        .width = spec.width,
    });
    static_cast<InteractionResult &>(result) = segments;
    result.changed = segments.changed;
    result.selected_index = segments.selected_index;
    if (spec.draw_panel && Available(spec.tabs[result.selected_index])) {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() -
                           ImGui::GetStyle().ItemSpacing.y);
      ImGui::PushID(detail::Owned(spec.id).c_str());
      ImGui::PushID("panel");
      spec.draw_panel(result.selected_index);
      ImGui::PopID();
      ImGui::PopID();
    }
    return result;
  }

  std::vector<ImGuiID> item_ids(spec.tabs.size());
  std::optional<std::size_t> focus_target =
      spec.request_focus ? std::optional{selected} : std::nullopt;
  ImGui::PushID(detail::Owned(spec.id).c_str());
  if (ImGui::BeginTabBar("##tab-set",
                         ImGuiTabBarFlags_FittingPolicyResizeDown)) {
    for (std::size_t index = 0; index < spec.tabs.size(); ++index) {
      const ChoiceSpec &tab = spec.tabs[index];
      const bool available = Available(tab);
      ImGui::PushID(detail::Owned(tab.id).c_str());
      ImGui::BeginDisabled(!available);
      const std::string label =
          detail::Owned(tab.label) + "###" + detail::Owned(tab.id);
      const ImGuiTabItemFlags flags = index == selected
                                          ? ImGuiTabItemFlags_SetSelected
                                          : ImGuiTabItemFlags_None;
      const bool active = ImGui::BeginTabItem(label.c_str(), nullptr, flags);
      item_ids[index] = ImGui::GetItemID();
      const InteractionResult interaction = detail::CaptureInteraction();
      result.hovered = result.hovered || interaction.hovered;
      result.focused = result.focused || interaction.focused;
      result.active = result.active || interaction.active;

      if (available && ImGui::IsItemClicked() && index != selected) {
        result.changed = true;
        result.selected_index = index;
      }
      if (interaction.focused && available) {
        std::optional<std::size_t> target;
        if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) {
          target = BoundaryAvailable(spec.tabs, false);
        } else if (ImGui::IsKeyPressed(ImGuiKey_End, false)) {
          target = BoundaryAvailable(spec.tabs, true);
        } else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
          target = AdjacentAvailable(spec.tabs, index, -1);
        } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
          target = AdjacentAvailable(spec.tabs, index, 1);
        }
        if (target.has_value()) {
          result.changed = *target != selected;
          result.selected_index = *target;
          focus_target = *target;
        }
      }
      if (interaction.hovered && !tab.tooltip.empty()) {
        detail::ShowTooltip(tab.tooltip);
      }
      if (active) {
        ImGui::EndTabItem();
      }
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::EndTabBar();
  }
  if (spec.draw_panel && Available(spec.tabs[result.selected_index])) {
    ImGui::PushID("panel");
    spec.draw_panel(result.selected_index);
    ImGui::PopID();
  }
  if (focus_target.has_value() && item_ids[*focus_target] != 0) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    ImGui::SetFocusID(item_ids[*focus_target], window);
    ImGui::FocusWindow(window);
  }
  ImGui::PopID();
  return result;
}

} // namespace fancy_ui
