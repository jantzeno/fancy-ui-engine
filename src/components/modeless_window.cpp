#include "fancy_ui/components/modeless_window.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace fancy_ui {

ModelessWindowResult ModelessWindow(const ModelessWindowSpec &spec,
                                    ModelessWindowState state) {
  ModelessWindowResult result;
  const bool should_open = spec.request_open && !state.open;
  if (should_open) {
    state.open = true;
    state.restore_focus = false;
    result.opened = true;
  }
  if (spec.request_close) {
    state.open = false;
  }

  if (state.open) {
    const float scale = CurrentUiScale();
    const ImVec2 minimum(Scale(spec.minimum_size.x),
                         Scale(spec.minimum_size.y));
    const ImVec2 maximum(std::max(minimum.x, Scale(spec.maximum_size.x)),
                         std::max(minimum.y, Scale(spec.maximum_size.y)));
    ImGui::SetNextWindowSizeConstraints(minimum, maximum);
    if (should_open && state.size.x <= 0.0f) {
      ImGui::SetNextWindowSize(
          ImVec2(Scale(spec.initial_size.x), Scale(spec.initial_size.y)),
          ImGuiCond_Appearing);
    } else if (should_open) {
      ImGui::SetNextWindowSize(ImVec2(Scale(state.size.x), Scale(state.size.y)),
                               ImGuiCond_Appearing);
    }
    if (should_open && (state.position.x != 0.0f || state.position.y != 0.0f)) {
      ImGui::SetNextWindowPos(
          ImVec2(Scale(state.position.x), Scale(state.position.y)),
          ImGuiCond_Appearing);
    }

    bool open = true;
    const std::string title =
        detail::Owned(spec.title) + "###" + detail::Owned(spec.id);
    if (ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoCollapse)) {
      if (spec.draw_content) {
        spec.draw_content();
      }
      if (spec.draw_footer) {
        ImGui::Separator();
        spec.draw_footer();
      }
    }
    const ImVec2 position = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    ImGui::End();

    const Vec2 logical_position{position.x / scale, position.y / scale};
    const Vec2 logical_size{size.x / scale, size.y / scale};
    result.geometry_changed =
        std::abs(logical_position.x - state.position.x) > 0.5f ||
        std::abs(logical_position.y - state.position.y) > 0.5f ||
        std::abs(logical_size.x - state.size.x) > 0.5f ||
        std::abs(logical_size.y - state.size.y) > 0.5f;
    state.position = logical_position;
    state.size = logical_size;
    state.open = open;
  }

  if (state.was_open && !state.open) {
    result.closed = true;
    state.restore_focus = true;
    ImGui::SetKeyboardFocusHere(-1);
  }
  state.was_open = state.open;
  result.state = state;
  return result;
}

} // namespace fancy_ui
