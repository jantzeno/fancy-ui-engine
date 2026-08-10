#include "fancy_ui/components/renamable_select.hpp"

#include "fancy_ui/components/button.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>
#include <vector>

namespace fancy_ui {

RenamableSelectResult RenamableSelect(const RenamableSelectSpec &spec,
                                      RenamableSelectState &state) {
  RenamableSelectResult result;
  const std::size_t selected =
      spec.options.empty()
          ? 0
          : std::min(spec.selected_index, spec.options.size() - std::size_t{1});
  result.selected_index = selected;
  result.value = spec.options.empty()
                     ? std::string{}
                     : detail::Owned(spec.options[selected].label);

  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  ImGui::PushID(detail::Owned(spec.id).c_str());
  const detail::FieldLayout layout = detail::BeginFieldLayout(spec.label);
  detail::PushFieldControlState(spec.availability, spec.validation);
  detail::BeginAvailability(spec.availability);

  if (state.renaming) {
    std::vector<char> buffer(512, '\0');
    std::copy_n(state.draft.data(),
                std::min(state.draft.size(), buffer.size() - 1), buffer.data());
    ImGui::SetNextItemWidth(-Scale(40.0f));
    const bool entered =
        ImGui::InputText("##rename", buffer.data(), buffer.size(),
                         ImGuiInputTextFlags_EnterReturnsTrue);
    const InteractionResult interaction = detail::CaptureInteraction();
    static_cast<InteractionResult &>(result) = interaction;
    const bool edited = std::string_view(buffer.data()) != state.draft;
    if (edited) {
      state.draft = buffer.data();
    }
    const bool cancel = ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    const bool commit = entered || ImGui::IsItemDeactivatedAfterEdit();
    detail::DrawFocusRing(interaction, spec.validation.invalid);
    if (cancel || commit) {
      result.cancelled = cancel;
      result.committed = commit && !cancel;
      result.changed = result.committed && state.draft != state.original;
      result.value = cancel ? state.original : state.draft;
      state.renaming = false;
      state.restore_focus = true;
    } else {
      result.changed = edited;
      result.value = state.draft;
    }
  } else {
    if (state.restore_focus) {
      ImGui::SetKeyboardFocusHere();
      state.restore_focus = false;
    }
    const std::string preview = result.value;
    ImGui::SetNextItemWidth(-Scale(88.0f));
    const bool open = ImGui::BeginCombo("##value", preview.c_str());
    const InteractionResult interaction = detail::CaptureInteraction();
    static_cast<InteractionResult &>(result) = interaction;
    detail::DrawFocusRing(interaction, spec.validation.invalid);
    if (open) {
      for (std::size_t index = 0; index < spec.options.size(); ++index) {
        const SelectOption &option = spec.options[index];
        const bool enabled = option.enabled && option.availability.enabled &&
                             !option.availability.busy;
        ImGui::PushID(detail::Owned(option.id).c_str());
        ImGui::BeginDisabled(!enabled);
        if (ImGui::Selectable(detail::Owned(option.label).c_str(),
                              index == selected) &&
            enabled) {
          result.selection_changed = index != selected;
          result.selected_index = index;
          result.value = detail::Owned(option.label);
        }
        ImGui::EndDisabled();
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    const Availability rename_availability{
        .enabled = spec.availability.enabled &&
                   spec.rename_availability.enabled && !spec.options.empty(),
        .busy = spec.availability.busy || spec.rename_availability.busy,
        .reason = spec.rename_availability.reason,
    };
    if (Button({
                   .id = "rename",
                   .label = "Rename",
                   .variant = ButtonVariant::Tertiary,
                   .availability = rename_availability,
                   .size = {.x = 80.0f, .y = 28.0f},
               })
            .activated) {
      state.renaming = true;
      state.original = result.value;
      state.draft = result.value;
      result.rename_started = true;
    }
  }

  detail::EndAvailability(spec.availability, spec.tooltip);
  detail::PopFieldControlState(spec.availability, spec.validation);
  detail::EndFieldLayout(layout, spec.validation);
  ImGui::PopID();
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
