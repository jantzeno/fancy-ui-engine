#include "fancy_ui/components/color_picker_popup.hpp"

#include "fancy_ui/components/button.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>

namespace fancy_ui {

namespace {

ColorRgba ClampColor(const ColorRgba color) {
  return {
      .red = std::clamp(color.red, 0.0f, 1.0f),
      .green = std::clamp(color.green, 0.0f, 1.0f),
      .blue = std::clamp(color.blue, 0.0f, 1.0f),
      .alpha = std::clamp(color.alpha, 0.0f, 1.0f),
  };
}

float ColorPickerPopupWidth(const ColorPickerPopupSpec &spec) {
  const ImGuiStyle &style = ImGui::GetStyle();
  const float picker_width = Scale(260.0f);
  float content_width = picker_width;
  if (spec.layout == ColorPickerLayout::CurrentAndOriginal) {
    content_width += style.ItemInnerSpacing.x + ImGui::GetFrameHeight() * 3.0f;
  }
  content_width = std::max(
      content_width, ImGui::CalcTextSize(detail::Owned(spec.title).c_str()).x);
  content_width =
      std::max(content_width, Scale(72.0f * 2.0f) + style.ItemSpacing.x);
  return content_width + style.WindowPadding.x * 2.0f;
}

} // namespace

ColorPickerPopupResult ColorPickerPopup(const ColorPickerPopupSpec &spec,
                                        ColorPickerState &state) {
  ColorPickerPopupResult result;
  result.value = spec.value;
  const bool was_editing = state.editing;

  ImGui::PushID(detail::Owned(spec.id).c_str());
  if (spec.request_open) {
    state.editing = true;
    state.restore_focus = false;
    state.original = ClampColor(spec.value);
    state.draft = state.original;
    ImGui::OpenPopup("##color-picker");
    result.opened = true;
  }

  const float popup_width = ColorPickerPopupWidth(spec);
  ImGui::SetNextWindowSizeConstraints(ImVec2(popup_width, 0.0f),
                                      ImVec2(popup_width, FLT_MAX));
  if (ImGui::BeginPopup("##color-picker")) {
    state.editing = true;
    ImGui::TextUnformatted(detail::Owned(spec.title).c_str());
    ImGui::Separator();
    std::array<float, 4> draft{
        state.draft.red,
        state.draft.green,
        state.draft.blue,
        state.draft.alpha,
    };
    const std::array<float, 4> original{
        state.original.red,
        state.original.green,
        state.original.blue,
        state.original.alpha,
    };
    ImGuiColorEditFlags flags =
        ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoLabel;
    if (spec.layout == ColorPickerLayout::Compact) {
      flags |= ImGuiColorEditFlags_NoSidePreview;
    }
    if (spec.show_alpha) {
      flags |= ImGuiColorEditFlags_AlphaBar;
    } else {
      flags |= ImGuiColorEditFlags_NoAlpha;
    }
    ImGui::SetNextItemWidth(Scale(260.0f));
    if (ImGui::ColorPicker4("##value", draft.data(), flags, original.data())) {
      state.draft = ClampColor({
          .red = draft[0],
          .green = draft[1],
          .blue = draft[2],
          .alpha = spec.show_alpha ? draft[3] : state.original.alpha,
      });
    }

    const bool window_focused =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    bool commit = window_focused && ImGui::IsKeyPressed(ImGuiKey_Enter, false);
    bool cancel = window_focused && ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    commit |= Button({
                         .id = "apply",
                         .label = "Apply",
                         .variant = ButtonVariant::Primary,
                         .size = {.x = 72.0f, .y = 28.0f},
                     })
                  .activated;
    ImGui::SameLine();
    cancel |= Button({
                         .id = "cancel",
                         .label = "Cancel",
                         .size = {.x = 72.0f, .y = 28.0f},
                     })
                  .activated;

    if (cancel) {
      result.cancelled = true;
      result.value = state.original;
      state.editing = false;
      state.restore_focus = true;
      ImGui::CloseCurrentPopup();
    } else if (commit) {
      result.changed = state.draft != state.original;
      result.committed = true;
      result.value = state.draft;
      state.editing = false;
      state.restore_focus = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  } else if (was_editing && !spec.request_open && state.editing) {
    result.cancelled = true;
    result.value = state.original;
    state.editing = false;
    state.restore_focus = true;
  }
  result.picker_open = state.editing;
  ImGui::PopID();
  return result;
}

} // namespace fancy_ui
