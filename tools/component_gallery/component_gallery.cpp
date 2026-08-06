#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/application_chrome.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace fancy_ui::gallery {

namespace {

void Heading(const char *title, ImFont *font) {
  if (font != nullptr) {
    ImGui::PushFont(font, Scale(24.0f));
  }
  ImGui::TextUnformatted(title);
  if (font != nullptr) {
    ImGui::PopFont();
  }
}

void GalleryCard(const char *id, const char *title, ImFont *heading_font,
                 const std::function<void()> &draw,
                 const bool scrollable = false) {
  ImGui::TableNextColumn();
  ImGui::PushID(id);
  ImGui::PushStyleColor(
      ImGuiCol_ChildBg,
      ImVec4(CurrentPalette().surface.red, CurrentPalette().surface.green,
             CurrentPalette().surface.blue, CurrentPalette().surface.alpha));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(Scale(12.0f), Scale(8.0f)));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
  if (ImGui::BeginChild("##card", ImVec2(Scale(302.0f), Scale(220.0f)),
                        ImGuiChildFlags_Borders,
                        scrollable ? ImGuiWindowFlags_NoSavedSettings
                                   : ImGuiWindowFlags_NoScrollbar)) {
    Heading(title, heading_font);
    draw();
  }
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
  ImGui::PopID();
}

ButtonResult PreviewButton(const char *id, const char *label,
                           const detail::InteractionPreview preview) {
  const detail::ScopedInteractionPreview state(preview);
  return Button({
      .id = id,
      .label = label,
      .size = {.x = 134.0f, .y = 32.0f},
  });
}

void DrawButtons() {
  static_cast<void>(
      PreviewButton("default", "Default", detail::InteractionPreview::Rest));
  ImGui::SameLine();
  static_cast<void>(
      PreviewButton("hovered", "Hovered", detail::InteractionPreview::Hovered));
  static_cast<void>(
      PreviewButton("pressed", "Pressed", detail::InteractionPreview::Pressed));
  ImGui::SameLine();
  static_cast<void>(
      PreviewButton("focused", "Focused", detail::InteractionPreview::Focused));
}

void DrawAvailability(GalleryState &state) {
  const ButtonResult selected = Button({
      .id = "selected",
      .label = "Selected",
      .selected = state.availability_selected,
      .size = {.x = 134.0f, .y = 32.0f},
  });
  if (selected.activated) {
    state.availability_selected = !state.availability_selected;
  }
  ImGui::SameLine();
  static_cast<void>(Button({
      .id = "disabled",
      .label = "Disabled",
      .availability =
          {
              .enabled = false,
              .reason = "Select an eligible object",
          },
      .size = {.x = 134.0f, .y = 32.0f},
  }));
  const ButtonResult invalid = Button({
      .id = "invalid",
      .label = "Invalid",
      .validation =
          {
              .invalid = true,
          },
      .size = {.x = 134.0f, .y = 32.0f},
  });
  ImGui::SameLine();
  static_cast<void>(Button({
      .id = "busy",
      .label = "Busy",
      .availability = {.busy = true},
      .size = {.x = 134.0f, .y = 32.0f},
  }));
  ImGui::TextDisabled(
      "%s", invalid.activated ? "Invalid control activated; validation remains."
                              : "Disabled - select an eligible object");
}

void DrawWorkspaceSwitcher(detail::ApplicationChrome &chrome,
                           GalleryState &state) {
  using steppenface::WorkspaceKind;
  const bool model = state.component_workspace == WorkspaceKind::Model3d;
  ImGui::TextDisabled("%s · selected", model ? "3D" : "Canvas");
  ImGui::TextDisabled("%s · rest", model ? "Canvas" : "3D");
  chrome.DrawWorkspaceSwitcher({.active_workspace = state.component_workspace},
                               {.activate_workspace =
                                    [&state](const WorkspaceKind workspace) {
                                      state.component_workspace = workspace;
                                    }},
                               {}, 138.0f);
}

void DrawSelectionScope(detail::ApplicationChrome &chrome,
                        GalleryState &state) {
  using steppenface::SelectionScope;
  const bool canvas = state.component_selection_scope == SelectionScope::Canvas;
  ImGui::TextDisabled("%s · selected", canvas ? "Canvas" : "Object");
  ImGui::TextDisabled("%s · hover", canvas ? "Object" : "Canvas");
  const steppenface::ToolbarSegmentedView segmented{
      .id = {.value = "component.selection-scope"},
      .choices =
          {
              {.id = {.value = "component.scope.canvas"},
               .label = "Canvas",
               .selected = canvas,
               .action = {.field = {.value = "component.selection-scope"},
                          .value = SelectionScope::Canvas}},
              {.id = {.value = "component.scope.object"},
               .label = "Object",
               .selected = !canvas,
               .action = {.field = {.value = "component.selection-scope"},
                          .value = SelectionScope::Object}},
          },
  };
  const std::array previews{
      canvas ? detail::InteractionPreview::Rest
             : detail::InteractionPreview::Hovered,
      canvas ? detail::InteractionPreview::Hovered
             : detail::InteractionPreview::Rest,
  };
  chrome.DrawToolbarSegmented(
      segmented,
      {.commit_action =
           [&state](const steppenface::ControlActionView &action) {
             if (const auto *scope =
                     std::get_if<SelectionScope>(&action.value)) {
               state.component_selection_scope = *scope;
             }
           }},
      previews, 276.0f);
}

void DrawSelectionTool(detail::ApplicationChrome &chrome, GalleryState &state) {
  using steppenface::SelectionTool;
  ImGui::TextDisabled("Pointer · selected");
  ImGui::TextDisabled("Rectangle · pressed · Oval · disabled");
  const steppenface::ToolbarSegmentedView segmented{
      .id = {.value = "component.selection-tool"},
      .choices =
          {
              {.id = {.value = "component.tool.pointer"},
               .label = "Pointer",
               .selected =
                   state.component_selection_tool == SelectionTool::Pointer,
               .action = {.field = {.value = "component.selection-tool"},
                          .value = SelectionTool::Pointer}},
              {.id = {.value = "component.tool.rectangle"},
               .label = "Rectangle",
               .selected =
                   state.component_selection_tool == SelectionTool::Rectangle,
               .action = {.field = {.value = "component.selection-tool"},
                          .value = SelectionTool::Rectangle}},
              {.id = {.value = "component.tool.oval"},
               .label = "Oval",
               .selected =
                   state.component_selection_tool == SelectionTool::Oval,
               .action =
                   {.field = {.value = "component.selection-tool"},
                    .value = SelectionTool::Oval,
                    .availability = {.enabled = false,
                                     .disabled_reason =
                                         "The owning product view supplies the "
                                         "unavailable reason"}}},
          },
  };
  constexpr std::array previews{
      detail::InteractionPreview::Rest,
      detail::InteractionPreview::Pressed,
      detail::InteractionPreview::Rest,
  };
  chrome.DrawToolbarSegmented(
      segmented,
      {.commit_action =
           [&state](const steppenface::ControlActionView &action) {
             if (const auto *tool = std::get_if<SelectionTool>(&action.value)) {
               state.component_selection_tool = *tool;
             }
           }},
      previews, 276.0f);
}

void DrawInputs(GalleryState &state) {
  const detail::ScopedFieldLayoutPreview layout(Scale(76.0f));
  const NumericInputResult spacing = NumericInput({
      .id = "spacing",
      .label = "Spacing",
      .unit = "mm",
      .value = state.spacing,
      .minimum = 0.0,
      .format = "%.1f",
  });
  if (spacing.changed) {
    state.spacing = spacing.value;
  }
  static constexpr std::array options{
      SelectOption{.id = "quarters", .label = "0°, 90°"},
      SelectOption{.id = "every-quarter", .label = "Every 90°"},
  };
  const SelectResult rotation = Select({
      .id = "rotation",
      .label = "Rotation",
      .options = options,
      .selected_index = state.rotation_option,
  });
  if (rotation.changed) {
    state.rotation_option = rotation.selected_index;
  }
  const DurationResult duration = Duration({
      .id = "duration",
      .label = "Duration",
      .hours = state.hours,
      .minutes = state.minutes,
  });
  if (duration.changed) {
    state.hours = duration.hours;
    state.minutes = duration.minutes;
  }
}

void DrawSlider(GalleryState &state) {
  const detail::ScopedFieldLayoutPreview layout(Scale(76.0f));
  const SliderResult result = Slider({
      .id = "explode",
      .label = "Explode",
      .unit = "%",
      .value = state.explode,
      .minimum = 0.0f,
      .maximum = 100.0f,
      .format = "%.0f",
  });
  if (result.changed) {
    state.explode = result.value;
  }
  ImGui::Spacing();
  if (ImGui::BeginTable("##slider-scale", 3,
                        ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    ImGui::TextDisabled("0%%");
    ImGui::TableNextColumn();
    ImGui::TextDisabled("Exact value");
    ImGui::TableNextColumn();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                         ImGui::GetContentRegionAvail().x -
                         ImGui::CalcTextSize("100%").x);
    ImGui::TextDisabled("100%%");
    ImGui::EndTable();
  }
}

void DrawCompass(GalleryState &state, const bool inherited) {
  const RotationCompassResult result = RotationCompass({
      .id = inherited ? "inherited" : "local",
      .label = inherited ? "Allowed rotations" : "Rotations",
      .count = state.rotations,
      .inherited = inherited,
      .availability =
          inherited ? Availability{.enabled = false,
                                   .reason = "Inherited from bed settings"}
                    : Availability{},
  });
  if (result.changed) {
    state.rotations = result.count;
  }
}

void DrawCheckboxes(GalleryState &state) {
  const CheckboxResult checkbox = Checkbox({
      .id = "on",
      .label = "Enabled",
      .state = state.checkbox,
  });
  if (checkbox.changed) {
    state.checkbox = checkbox.state;
  }
  const CheckboxResult off = Checkbox({
      .id = "off",
      .label = "Off",
      .state = state.checkbox_off,
  });
  if (off.changed) {
    state.checkbox_off = off.state;
  }
  const CheckboxResult mixed = Checkbox({
      .id = "mixed",
      .label = "Mixed",
      .state = state.checkbox_mixed,
  });
  if (mixed.changed) {
    state.checkbox_mixed = mixed.state;
  }
  constexpr std::string_view unavailable_reason =
      "Controlled by the active profile";
  static_cast<void>(Checkbox({
      .id = "unavailable",
      .label = "Unavailable",
      .state = ToggleState::On,
      .availability =
          {
              .enabled = false,
              .reason = unavailable_reason,
          },
  }));
  ImGui::Indent(Scale(24.0f));
  ImGui::PushTextWrapPos();
  ImGui::TextDisabled("%.*s", static_cast<int>(unavailable_reason.size()),
                      unavailable_reason.data());
  ImGui::PopTextWrapPos();
  ImGui::Unindent(Scale(24.0f));
}

void DrawVisibility(detail::UiAssetAtlas &assets, GalleryState &state) {
  const detail::ScopedFieldLayoutPreview layout(Scale(76.0f));
  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");
  const VisibilityToggleResult result = VisibilityToggle({
      .id = "visible",
      .label = "Overlay",
      .state = state.visible,
      .visible_icon = visible,
      .hidden_icon = hidden,
  });
  if (result.changed) {
    state.visible = result.state;
  }
  const VisibilityToggleResult guides = VisibilityToggle({
      .id = "guides",
      .label = "Guides",
      .state = state.guides_visible,
      .visible_icon = visible,
      .hidden_icon = hidden,
  });
  if (guides.changed) {
    state.guides_visible = guides.state;
  }
  const VisibilityToggleResult selection = VisibilityToggle({
      .id = "mixed",
      .label = "Selection",
      .state = state.selection_visible,
      .visible_icon = visible,
      .hidden_icon = hidden,
  });
  if (selection.changed) {
    state.selection_visible = selection.state;
  }
}

void DrawEnabledLocked(detail::UiAssetAtlas &assets, GalleryState &state) {
  const detail::ScopedFieldLayoutPreview layout(Scale(76.0f));
  const IconPainter enabled = assets.Painter("success");
  const IconPainter disabled = assets.Painter("failure");
  const IconPainter locked = assets.Painter("orbit-locked");
  const IconPainter unlocked = assets.Painter("orbit-unlocked");
  const auto field = [](const char *id, const char *category,
                        const char *on_label, const char *off_label,
                        ToggleState &state, const IconPainter &on_icon,
                        const IconPainter &off_icon) {
    ImGui::PushFont(nullptr, Scale(21.0f));
    const detail::FieldLayout field_layout = detail::BeginFieldLayout(category);
    const CheckboxResult result = Checkbox({
        .id = id,
        .label = state == ToggleState::On ? on_label : off_label,
        .state = state,
        .on_icon = on_icon,
        .off_icon = off_icon,
        .show_checkbox = true,
    });
    detail::EndFieldLayout(field_layout, {});
    ImGui::PopFont();
    if (result.changed) {
      state = result.state;
    }
  };
  field("grain-enabled", "Grain", "Enabled", "Disabled", state.grain_enabled,
        enabled, disabled);
  field("grain-disabled", "Grain", "Enabled", "Disabled", state.grain_disabled,
        enabled, disabled);
  field("direction-locked", "Direction", "Locked", "Unlocked",
        state.direction_locked, locked, unlocked);
  field("direction-unlocked", "Direction", "Locked", "Unlocked",
        state.direction_unlocked, locked, unlocked);
}

template <std::size_t Size>
void ApplySelection(std::array<bool, Size> &selection, int &anchor,
                    const int index, const bool additive, const bool range) {
  if (range && anchor >= 0) {
    selection.fill(false);
    const int first = std::min(anchor, index);
    const int last = std::max(anchor, index);
    for (int selected = first; selected <= last; ++selected) {
      selection[static_cast<std::size_t>(selected)] = true;
    }
    return;
  }
  if (additive) {
    selection[static_cast<std::size_t>(index)] =
        !selection[static_cast<std::size_t>(index)];
  } else {
    selection.fill(false);
    selection[static_cast<std::size_t>(index)] = true;
  }
  anchor = index;
}

void DrawTreeColumnHeaders() {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float height = metrics.explorer.audit_columns_height;
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, height));
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddLine(ImVec2(minimum.x, maximum.y), maximum,
                     ImGui::GetColorU32(ImVec4(CurrentPalette().border.red,
                                               CurrentPalette().border.green,
                                               CurrentPalette().border.blue,
                                               CurrentPalette().border.alpha)),
                     metrics.geometry.border);

  ImFont *font = ImGui::GetFont();
  const float font_size = Scale(12.0f);
  const auto draw_label = [&](const std::string_view label,
                              const float center_x) {
    const ImVec2 size =
        font->CalcTextSizeA(font_size, std::numeric_limits<float>::max(), 0.0f,
                            label.data(), label.data() + label.size());
    draw_list->AddText(
        font, font_size,
        ImVec2(center_x - size.x * 0.5f,
               minimum.y + std::floor((height - size.y) * 0.5f)),
        ImGui::GetColorU32(ImVec4(CurrentPalette().text_secondary.red,
                                  CurrentPalette().text_secondary.green,
                                  CurrentPalette().text_secondary.blue,
                                  CurrentPalette().text_secondary.alpha)),
        label.data(), label.data() + label.size());
  };
  const float visibility_center =
      maximum.x - metrics.explorer.audit_visibility_column_width * 0.5f;
  const float action_center =
      maximum.x - metrics.explorer.audit_visibility_column_width -
      metrics.explorer.audit_action_column_width * 0.5f - Scale(4.0f);
  const float color_center = maximum.x -
                             metrics.explorer.audit_visibility_column_width -
                             metrics.explorer.audit_action_column_width -
                             metrics.explorer.audit_color_column_width * 0.5f;
  draw_label("Hierarchy",
             minimum.x + metrics.spacing.space03 +
                 font->CalcTextSizeA(font_size,
                                     std::numeric_limits<float>::max(), 0.0f,
                                     "Hierarchy")
                         .x *
                     0.5f);
  draw_label("Color", color_center);
  draw_label("Act", action_center);
  draw_label("Visibility", visibility_center);
}

void DrawTreeRows(detail::UiAssetAtlas &assets, GalleryState &state) {
  static constexpr std::array<std::string_view, 3> labels{
      "Bed 1",
      "Frame plate",
      "Path 184",
  };
  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");
  const IconPainter more = assets.Painter("more");
  const int restore_color_row =
      state.reference_tree_color_picker.restore_focus
          ? std::clamp(state.reference_tree_color_row, 0, 2)
          : -1;
  bool request_color_picker = false;
  const ToggleState bed_visibility =
      AggregateVisibility(state.reference_tree_visibility);

  DrawTreeColumnHeaders();
  {
    HierarchyTree tree;
    HierarchyRowResult bed;
    {
      const detail::ScopedInteractionPreview preview(
          detail::InteractionPreview::Hovered);
      bed = HierarchyRow(
          tree, {
                    .id = "reference-bed",
                    .label = labels[0],
                    .secondary_label = "6 objects",
                    .expandable = true,
                    .expanded = state.reference_bed_expanded,
                    .selected = state.reference_tree_selected[0],
                    .color = state.reference_tree_colors[0],
                    .color_tooltip = "Edit Bed 1 color",
                    .request_color_focus = restore_color_row == 0,
                    .action_icon = more,
                    .action_tooltip = "Bed 1 actions",
                    .visibility = bed_visibility,
                    .visible_icon = visible,
                    .hidden_icon = hidden,
                    .visibility_tooltip = "Show or hide all objects in Bed 1",
                });
    }
    if (bed.expansion_changed) {
      state.reference_bed_expanded = bed.expanded;
    }
    if (bed.activated) {
      ApplySelection(state.reference_tree_selected,
                     state.reference_tree_selection_anchor, 0, bed.additive,
                     bed.range);
      state.reference_tree_feedback = "Selected: Bed 1 · 6 objects";
    }
    if (bed.visibility_changed) {
      state.reference_tree_visibility.fill(bed.visibility);
      state.reference_tree_feedback = bed.visibility == ToggleState::On
                                          ? "Bed 1 objects are visible."
                                          : "Bed 1 objects are hidden.";
    }
    if (bed.color_activated) {
      state.reference_tree_color_row = 0;
      request_color_picker = true;
    }
    if (bed.action_activated) {
      state.reference_tree_action_row = 0;
      ImGui::OpenPopup("##reference-tree-actions");
    }

    if (bed.expanded) {
      const HierarchyRowResult frame = HierarchyRow(
          tree, {
                    .id = "reference-frame",
                    .label = labels[1],
                    .secondary_label = "Selected",
                    .expandable = true,
                    .expanded = state.reference_frame_expanded,
                    .selected = state.reference_tree_selected[1],
                    .color = state.reference_tree_colors[1],
                    .color_tooltip = "Edit Frame plate color",
                    .request_color_focus = restore_color_row == 1,
                    .action_icon = more,
                    .action_tooltip = "Frame plate actions",
                    .visibility = state.reference_tree_visibility[0],
                    .visible_icon = visible,
                    .hidden_icon = hidden,
                    .visibility_tooltip = "Show or hide Frame plate",
                });
      if (frame.expansion_changed) {
        state.reference_frame_expanded = frame.expanded;
      }
      if (frame.activated) {
        ApplySelection(state.reference_tree_selected,
                       state.reference_tree_selection_anchor, 1, frame.additive,
                       frame.range);
        state.reference_tree_feedback = "Selected: Frame plate";
      }
      if (frame.visibility_changed) {
        state.reference_tree_visibility[0] = frame.visibility;
        state.reference_tree_feedback = frame.visibility == ToggleState::On
                                            ? "Frame plate is visible."
                                            : "Frame plate is hidden.";
      }
      if (frame.color_activated) {
        state.reference_tree_color_row = 1;
        request_color_picker = true;
      }
      if (frame.action_activated) {
        state.reference_tree_action_row = 1;
        ImGui::OpenPopup("##reference-tree-actions");
      }

      if (frame.expanded) {
        HierarchyRowResult path;
        {
          const detail::ScopedInteractionPreview preview(
              detail::InteractionPreview::Focused);
          path = HierarchyRow(
              tree, {
                        .id = "reference-path",
                        .label = labels[2],
                        .secondary_label = "Invalid",
                        .selected = state.reference_tree_selected[2],
                        .status = SemanticStatus::Failure,
                        .color = state.reference_tree_colors[2],
                        .color_tooltip = "Edit Path 184 color",
                        .request_color_focus = restore_color_row == 2,
                        .action_icon = more,
                        .action_tooltip = "Path 184 actions",
                        .visibility = state.reference_tree_visibility[1],
                        .visible_icon = visible,
                        .hidden_icon = hidden,
                        .visibility_tooltip = "Show or hide Path 184",
                    });
        }
        if (path.activated) {
          ApplySelection(state.reference_tree_selected,
                         state.reference_tree_selection_anchor, 2,
                         path.additive, path.range);
          state.reference_tree_feedback = "Selected: Path 184 · Invalid";
        }
        if (path.visibility_changed) {
          state.reference_tree_visibility[1] = path.visibility;
          state.reference_tree_feedback = path.visibility == ToggleState::On
                                              ? "Path 184 is visible."
                                              : "Path 184 is hidden.";
        }
        if (path.color_activated) {
          state.reference_tree_color_row = 2;
          request_color_picker = true;
        }
        if (path.action_activated) {
          state.reference_tree_action_row = 2;
          ImGui::OpenPopup("##reference-tree-actions");
        }
        tree.Pop();
      }
      tree.Pop();
    }
  }
  if (restore_color_row >= 0) {
    state.reference_tree_color_picker.restore_focus = false;
  }

  const int color_row = std::clamp(state.reference_tree_color_row, 0, 2);
  const std::string picker_title =
      std::string(labels[static_cast<std::size_t>(color_row)]) + " color";
  const ColorPickerPopupResult color = ColorPickerPopup(
      {
          .id = "reference-tree-color",
          .title = picker_title,
          .value =
              state.reference_tree_colors[static_cast<std::size_t>(color_row)],
          .request_open = request_color_picker,
      },
      state.reference_tree_color_picker);
  if (color.changed) {
    state.reference_tree_colors[static_cast<std::size_t>(color_row)] =
        color.value;
    state.reference_tree_feedback =
        "Updated " + std::string(labels[static_cast<std::size_t>(color_row)]) +
        " color.";
  } else if (color.cancelled) {
    state.reference_tree_feedback = "Color edit cancelled.";
  }

  if (ImGui::BeginPopup("##reference-tree-actions")) {
    const int row = std::clamp(state.reference_tree_action_row, 0, 2);
    ImGui::TextUnformatted(
        std::string(labels[static_cast<std::size_t>(row)]).c_str());
    ImGui::Separator();
    if (ImGui::MenuItem("Frame in view")) {
      state.reference_tree_feedback =
          "Frame requested: " +
          std::string(labels[static_cast<std::size_t>(row)]);
    }
    if (ImGui::MenuItem("Inspect properties")) {
      state.reference_tree_feedback =
          "Inspection scope: " +
          std::string(labels[static_cast<std::size_t>(row)]);
    }
    ImGui::EndPopup();
  }
  ImGui::Spacing();
  ImGui::TextDisabled("%s", state.reference_tree_feedback.c_str());
}

void DrawHierarchyInteractionDemo(detail::UiAssetAtlas &assets,
                                  GalleryState &state) {
  static constexpr std::array<std::string_view, 3> labels{
      "Front housing",
      "Face plate",
      "Outer contour",
  };
  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");
  const IconPainter more = assets.Painter("more");
  bool request_color_picker = false;
  const ToggleState assembly_visibility =
      AggregateVisibility(state.tree_visibility);

  {
    HierarchyTree tree;
    const HierarchyRowResult assembly = HierarchyRow(
        tree, {
                  .id = "assembly",
                  .label = labels[0],
                  .secondary_label = assembly_visibility == ToggleState::Mixed
                                         ? "Assembly · mixed visibility"
                                         : "Assembly · 2 descendants",
                  .expandable = true,
                  .expanded = state.assembly_expanded,
                  .selected = state.tree_selected[0],
                  .action_icon = more,
                  .action_tooltip = "Front housing actions",
                  .visibility = assembly_visibility,
                  .visible_icon = visible,
                  .hidden_icon = hidden,
                  .visibility_tooltip = "Show or hide all descendants",
              });
    if (assembly.expansion_changed) {
      state.assembly_expanded = assembly.expanded;
    }
    if (assembly.activated) {
      ApplySelection(state.tree_selected, state.tree_selection_anchor, 0,
                     assembly.additive, assembly.range);
      state.tree_feedback = "Selected: Front housing · Assembly";
    }
    if (assembly.visibility_changed) {
      state.tree_visibility.fill(assembly.visibility);
      state.tree_feedback = assembly.visibility == ToggleState::On
                                ? "Front housing descendants are visible."
                                : "Front housing descendants are hidden.";
    }
    if (assembly.action_activated) {
      state.tree_action_row = 0;
      ImGui::OpenPopup("##tree-actions");
    }

    if (assembly.expanded) {
      const bool request_color_focus = state.tree_color_picker.restore_focus;
      const HierarchyRowResult part = HierarchyRow(
          tree, {
                    .id = "part",
                    .label = labels[1],
                    .secondary_label = "Part 4 · 1 path",
                    .expandable = true,
                    .expanded = state.part_expanded,
                    .selected = state.tree_selected[1],
                    .color = state.part_color,
                    .color_tooltip = "Edit Face plate color",
                    .request_color_focus = request_color_focus,
                    .action_icon = more,
                    .action_tooltip = "Face plate actions",
                    .visibility = state.tree_visibility[0],
                    .visible_icon = visible,
                    .hidden_icon = hidden,
                    .visibility_tooltip = "Show or hide Face plate",
                });
      if (request_color_focus) {
        state.tree_color_picker.restore_focus = false;
      }
      if (part.expansion_changed) {
        state.part_expanded = part.expanded;
      }
      if (part.activated) {
        ApplySelection(state.tree_selected, state.tree_selection_anchor, 1,
                       part.additive, part.range);
        state.tree_feedback = "Selected: Face plate · Part 4";
      }
      if (part.visibility_changed) {
        state.tree_visibility[0] = part.visibility;
        state.tree_feedback = part.visibility == ToggleState::On
                                  ? "Face plate is visible."
                                  : "Face plate is hidden.";
      }
      request_color_picker = part.color_activated;
      if (part.action_activated) {
        state.tree_action_row = 1;
        ImGui::OpenPopup("##tree-actions");
      }

      if (part.expanded) {
        const HierarchyRowResult path = HierarchyRow(
            tree, {
                      .id = "path",
                      .label = labels[2],
                      .secondary_label = "Path 184 · closed contour",
                      .selected = state.tree_selected[2],
                      .action_icon = more,
                      .action_tooltip = "Outer contour actions",
                      .visibility = state.tree_visibility[1],
                      .visible_icon = visible,
                      .hidden_icon = hidden,
                      .visibility_tooltip = "Show or hide Outer contour",
                  });
        if (path.activated) {
          ApplySelection(state.tree_selected, state.tree_selection_anchor, 2,
                         path.additive, path.range);
          state.tree_feedback = "Selected: Outer contour · Path 184";
        }
        if (path.visibility_changed) {
          state.tree_visibility[1] = path.visibility;
          state.tree_feedback = path.visibility == ToggleState::On
                                    ? "Outer contour is visible."
                                    : "Outer contour is hidden.";
        }
        if (path.action_activated) {
          state.tree_action_row = 2;
          ImGui::OpenPopup("##tree-actions");
        }
        tree.Pop();
      }
      tree.Pop();
    }
  }

  const ColorPickerPopupResult color = ColorPickerPopup(
      {
          .id = "tree-color",
          .title = "Face plate color",
          .value = state.part_color,
          .request_open = request_color_picker,
      },
      state.tree_color_picker);
  if (color.changed) {
    state.part_color = color.value;
    state.tree_feedback = "Updated Face plate color.";
  } else if (color.cancelled) {
    state.tree_feedback = "Face plate color edit cancelled.";
  }

  if (ImGui::BeginPopup("##tree-actions")) {
    const int row = std::clamp(state.tree_action_row, 0, 2);
    ImGui::TextUnformatted(
        std::string(labels[static_cast<std::size_t>(row)]).c_str());
    ImGui::Separator();
    if (ImGui::MenuItem("Frame in view")) {
      state.tree_feedback = "Frame requested: " +
                            std::string(labels[static_cast<std::size_t>(row)]);
    }
    if (ImGui::MenuItem("Inspect properties")) {
      state.tree_feedback = "Inspection scope: " +
                            std::string(labels[static_cast<std::size_t>(row)]);
    }
    ImGui::EndPopup();
  }
  ImGui::Spacing();
  ImGui::TextDisabled("%s", state.tree_feedback.c_str());
}

void DrawIssueHierarchy(detail::UiAssetAtlas &assets, GalleryState &state) {
  const detail::ScopedFieldLayoutPreview layout(Scale(96.0f));
  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");

  ImGui::PushFont(nullptr, Scale(21.0f));
  const detail::FieldLayout field_layout =
      detail::BeginFieldLayout("Issue labels");
  const CheckboxResult labels = Checkbox(
      {.id = "show-labels",
       .label = "Show on Canvas",
       .state = state.show_issue_labels ? ToggleState::On : ToggleState::Off,
       .show_checkbox = true});
  detail::EndFieldLayout(field_layout, {});
  ImGui::PopFont();
  if (labels.changed) {
    state.show_issue_labels = labels.state == ToggleState::On;
  }

  const ToggleState repairable_visibility =
      AggregateVisibility(state.issue_visibility);
  {
    InformationTree tree;
    const InformationTreeRowResult repairable = InformationTreeRow(
        tree, {
                  .id = "repairable",
                  .label = "Repairable",
                  .value = "5",
                  .expandable = true,
                  .expanded = state.repairable_expanded,
                  .status = SemanticStatus::Information,
                  .visibility = repairable_visibility,
                  .visible_icon = visible,
                  .hidden_icon = hidden,
                  .visibility_tooltip = "Show or hide all repairable issues",
              });
    if (repairable.expansion_changed) {
      state.repairable_expanded = repairable.expanded;
    }
    if (repairable.visibility_changed) {
      state.issue_visibility.fill(repairable.visibility);
    }
    if (repairable.expanded) {
      const InformationTreeRowResult open_contours = InformationTreeRow(
          tree, {
                    .id = "open-contours",
                    .label = "Open contours",
                    .value = "4",
                    .status = SemanticStatus::Information,
                    .visibility = state.issue_visibility[0],
                    .visible_icon = visible,
                    .hidden_icon = hidden,
                    .visibility_tooltip = "Show or hide open contours",
                });
      if (open_contours.visibility_changed) {
        state.issue_visibility[0] = open_contours.visibility;
      }
      const InformationTreeRowResult orphan_hole = InformationTreeRow(
          tree, {
                    .id = "orphan-hole",
                    .label = "Orphan hole",
                    .value = "1",
                    .status = SemanticStatus::Information,
                    .visibility = state.issue_visibility[1],
                    .visible_icon = visible,
                    .hidden_icon = hidden,
                    .visibility_tooltip = "Show or hide the orphan hole",
                });
      if (orphan_hole.visibility_changed) {
        state.issue_visibility[1] = orphan_hole.visibility;
      }
      tree.Pop();
    }
  }
}

void DrawStatusTypes(detail::UiAssetAtlas &assets) {
  const auto status = [&assets](const char *id, const char *message,
                                const SemanticStatus tone, const char *icon) {
    StatusCard({
        .id = id,
        .message = message,
        .status = tone,
        .icon = assets.Painter(icon),
    });
  };
  if (ImGui::BeginTable("##status-grid", 2,
                        ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    status("info", "2 files selected", SemanticStatus::Information,
           "information");
    ImGui::TableNextColumn();
    status("success", "Ready to search", SemanticStatus::Success, "success");
    ImGui::TableNextColumn();
    status("warning", "4 open paths", SemanticStatus::Warning, "alert");
    ImGui::TableNextColumn();
    status("failure", "Search blocked", SemanticStatus::Failure, "failure");
    ImGui::EndTable();
  }
}

void DrawProgress() {
  ImGui::PushFont(nullptr, Scale(21.0f));
  ImGui::TextUnformatted("Determinate · 62%");
  ImGui::PopFont();
  ProgressBar({
      .id = "determinate",
      .label = "Search progress: 62%",
      .value = 0.62f,
      .status = SemanticStatus::Busy,
  });
  ImGui::Spacing();
  ImGui::PushFont(nullptr, Scale(21.0f));
  ImGui::TextUnformatted("Indeterminate");
  ImGui::PopFont();
  ProgressBar({
      .id = "indeterminate",
      .label = "Preparing geometry",
      .value = std::nullopt,
      .status = SemanticStatus::Busy,
  });
  ImGui::TextDisabled("Preparing geometry...");
}

void DrawOperation(detail::UiAssetAtlas &assets) {
  if (ImGui::BeginTable("##operation-status", 2,
                        ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    StatusCard({.id = "preview",
                .message = "8 pieces ready",
                .status = SemanticStatus::Preview,
                .icon = assets.Painter("visibility")});
    ImGui::TableNextColumn();
    StatusCard({.id = "busy",
                .message = "Iteration 24",
                .status = SemanticStatus::Busy,
                .icon = assets.Painter("busy")});
    ImGui::EndTable();
  }
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ImFont *font = ImGui::GetFont();
  const auto intrinsic_button_width = [font](const std::string_view label) {
    return font->CalcTextSizeA(Scale(21.0f), std::numeric_limits<float>::max(),
                               0.0f, label.data(), label.data() + label.size())
               .x +
           ImGui::GetStyle().FramePadding.x * 2.0f;
  };
  const float gap = metrics.spacing.space03;
  const float progress_width = std::max(
      metrics.geometry.progress_height,
      ImGui::GetContentRegionAvail().x - intrinsic_button_width("Pause") -
          intrinsic_button_width("Stop") - gap * 2.0f);
  const float row_y = ImGui::GetCursorScreenPos().y;
  ImGui::SetCursorScreenPos(
      ImVec2(ImGui::GetCursorScreenPos().x,
             row_y + std::floor((metrics.geometry.compact_target -
                                 metrics.geometry.progress_height) *
                                0.5f)));
  ProgressBar({.id = "search-progress",
               .label = "Search progress: 62%",
               .value = 0.62f,
               .status = SemanticStatus::Busy,
               .size = {.x = progress_width / CurrentUiScale()}});
  ImGui::SameLine(0.0f, gap);
  ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, row_y));
  static_cast<void>(
      Button({.id = "pause",
              .label = "Pause",
              .size = {.y = LogicalLayoutMetrics().geometry.compact_target}}));
  ImGui::SameLine(0.0f, gap);
  ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, row_y));
  static_cast<void>(
      Button({.id = "stop",
              .label = "Stop",
              .size = {.y = LogicalLayoutMetrics().geometry.compact_target}}));
}

void DrawEmptyOverflow() {
  EmptyState({
      .id = "empty",
      .title = "No matching items.",
  });
  ImGui::Spacing();
  static_cast<void>(ValueDisplay({
      .id = "overflow",
      .label = "Long content",
      .value = "Front-housing-outline-final-repaired.svg",
  }));
}

void DrawColorPickers(GalleryState &state) {
  const std::span<const ColorRgba> preview_color(&state.preview_picker_color,
                                                 1);
  const ColorSwatchResult preview = ColorSwatch(
      {
          .id = "preview",
          .label = "Current and original",
          .tooltip = "Open full color picker",
          .picker_title = "Current and original",
          .value = state.preview_picker_color,
          .colors = preview_color,
          .picker_layout = ColorPickerLayout::CurrentAndOriginal,
      },
      state.preview_picker);
  if (preview.changed) {
    state.preview_picker_color = preview.value;
  }

  const std::span<const ColorRgba> compact_color(&state.compact_picker_color,
                                                 1);
  const ColorSwatchResult compact = ColorSwatch(
      {
          .id = "compact",
          .label = "Compact",
          .tooltip = "Open compact color picker",
          .picker_title = "Compact color",
          .value = state.compact_picker_color,
          .colors = compact_color,
          .picker_layout = ColorPickerLayout::Compact,
      },
      state.compact_picker);
  if (compact.changed) {
    state.compact_picker_color = compact.value;
  }
}

} // namespace

void DrawHierarchySample(detail::UiAssetAtlas &assets, GalleryState &state) {
  DrawHierarchyInteractionDemo(assets, state);
}

void DrawComponentGallery(detail::UiAssetAtlas &assets, GalleryState &state) {
  ApplyTheme(state.theme, state.scale);
  assets.InstallPendingIcons();

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  const bool shell_preview = state.active_tab == GalleryTab::Shell;
  if (shell_preview) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  } else {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(24.0f), Scale(24.0f)));
  }
  ImGui::Begin("Fancy UI component gallery", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);
  if (shell_preview) {
    ImGui::PopStyleVar(2);
    const bool escape_owned_at_frame_start =
        ImGui::IsAnyItemActive() ||
        ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId |
                                   ImGuiPopupFlags_AnyPopupLevel);
    const bool return_requested = DrawApplicationShellGallery(assets, state);
    const bool escape_requested = ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    ImGui::End();
    if (return_requested ||
        (escape_requested && !escape_owned_at_frame_start)) {
      LeaveShellPreview(state);
    }
    return;
  }
  ImGui::PopStyleVar();

  if (assets.bold_font() != nullptr) {
    ImGui::PushFont(assets.bold_font(), Scale(29.0f));
  }
  ImGui::TextUnformatted("Fancy UI gallery");
  if (assets.bold_font() != nullptr) {
    ImGui::PopFont();
  }
  ImGui::SameLine();
  ImGui::TextDisabled("- Fancy UI - %.0f%%", state.scale * 100.0f);
  ImGui::SameLine(ImGui::GetWindowWidth() - Scale(184.0f));
  if (Button({
                 .id = "theme-light",
                 .label = "Light",
                 .selected = state.theme == ResolvedTheme::Light,
                 .size = {.x = 76.0f, .y = 28.0f},
             })
          .activated) {
    state.theme = ResolvedTheme::Light;
    state.settings.system_theme = ResolvedTheme::Light;
  }
  ImGui::SameLine();
  if (Button({
                 .id = "theme-dark",
                 .label = "Dark",
                 .selected = state.theme == ResolvedTheme::Dark,
                 .size = {.x = 76.0f, .y = 28.0f},
             })
          .activated) {
    state.theme = ResolvedTheme::Dark;
    state.settings.system_theme = ResolvedTheme::Dark;
  }
  ImGui::PushFont(nullptr, Scale(21.0f));
  ImGui::TextDisabled(
      "Canonical light/dark parity; pointer and keyboard states remain live.");
  ImGui::PopFont();
  ImGui::Spacing();

  const auto move_tab = [&state](const int delta) {
    const int current = static_cast<int>(state.active_tab);
    ActivateGalleryTab(
        state, static_cast<GalleryTab>((current + delta + kGalleryTabCount) %
                                       kGalleryTabCount));
  };
  if (ImGui::BeginTabBar("##gallery-tabs",
                         ImGuiTabBarFlags_FittingPolicyResizeDown)) {
    const auto tab = [&](const char *label, const GalleryTab gallery_tab,
                         const std::function<void()> &draw) {
      const ImGuiTabItemFlags flags = state.active_tab == gallery_tab
                                          ? ImGuiTabItemFlags_SetSelected
                                          : ImGuiTabItemFlags_None;
      const bool restore_focus =
          state.focus_active_tab && state.active_tab == gallery_tab;
      if (restore_focus) {
        ImGui::SetKeyboardFocusHere();
      }
      const bool visible = ImGui::BeginTabItem(label, nullptr, flags);
      if (restore_focus) {
        state.focus_active_tab = false;
      }
      const bool focused = ImGui::IsItemFocused();
      if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ActivateGalleryTab(state, gallery_tab);
      }
      if (focused && ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        move_tab(-1);
      } else if (focused && ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        move_tab(1);
      }
      if (visible) {
        draw();
        ImGui::EndTabItem();
      }
    };
    tab("Components", GalleryTab::Components, [&assets, &state] {
      detail::ApplicationChrome chrome(assets);
      ImGui::PushFont(nullptr, Scale(21.0f));
      ImGui::TextDisabled(
          "Shared controls, hierarchy rows, semantic feedback, and values.");
      ImGui::PopFont();
      ImGui::Spacing();
      const float table_width = Scale(4.0f * 302.0f + 3.0f * 8.0f);
      if (ImGui::BeginChild("##gallery-scroll", ImVec2(0.0f, 0.0f), false,
                            ImGuiWindowFlags_HorizontalScrollbar)) {
        if (ImGui::BeginTable("##component-grid", 4,
                              ImGuiTableFlags_SizingFixedFit,
                              ImVec2(table_width, 0.0f))) {
          for (int column = 0; column < 4; ++column) {
            ImGui::TableSetupColumn(
                "component", ImGuiTableColumnFlags_WidthFixed, Scale(302.0f));
          }
          GalleryCard("buttons", "Buttons", assets.bold_font(), DrawButtons);
          GalleryCard("availability", "Availability", assets.bold_font(),
                      [&state] { DrawAvailability(state); });
          GalleryCard(
              "workspace-switcher", "Workspace switcher", assets.bold_font(),
              [&chrome, &state] { DrawWorkspaceSwitcher(chrome, state); });
          GalleryCard("selection-scope", "Selection scope", assets.bold_font(),
                      [&chrome, &state] { DrawSelectionScope(chrome, state); });
          GalleryCard("selection-tool", "Selection tool", assets.bold_font(),
                      [&chrome, &state] { DrawSelectionTool(chrome, state); });
          GalleryCard("inputs", "Inputs", assets.bold_font(),
                      [&state] { DrawInputs(state); });
          GalleryCard("slider", "Slider", assets.bold_font(),
                      [&state] { DrawSlider(state); });
          GalleryCard("compass", "Compass", assets.bold_font(),
                      [&state] { DrawCompass(state, false); });
          GalleryCard("compass-inherited", "Compass inherited",
                      assets.bold_font(),
                      [&state] { DrawCompass(state, true); });
          GalleryCard("checkboxes", "Checkboxes", assets.bold_font(),
                      [&state] { DrawCheckboxes(state); });
          GalleryCard("visibility", "Visibility", assets.bold_font(),
                      [&assets, &state] { DrawVisibility(assets, state); });
          GalleryCard("enabled-locked", "Enabled & locked", assets.bold_font(),
                      [&assets, &state] { DrawEnabledLocked(assets, state); });
          GalleryCard("tree-rows", "Tree rows", assets.bold_font(),
                      [&assets, &state] { DrawTreeRows(assets, state); });
          GalleryCard(
              "issue-hierarchy", "Issue hierarchy", assets.bold_font(),
              [&assets, &state] { DrawIssueHierarchy(assets, state); }, true);
          GalleryCard("status-types", "Status types", assets.bold_font(),
                      [&assets] { DrawStatusTypes(assets); });
          GalleryCard("operation", "Operation", assets.bold_font(),
                      [&assets] { DrawOperation(assets); });
          GalleryCard("empty-overflow", "Empty & overflow", assets.bold_font(),
                      DrawEmptyOverflow);
          GalleryCard("color-pickers", "Color pickers", assets.bold_font(),
                      [&state] { DrawColorPickers(state); });
          GalleryCard("progress", "Progress", assets.bold_font(), DrawProgress);
          ImGui::EndTable();
        }
      }
      ImGui::EndChild();
    });
    tab("Application shell", GalleryTab::Shell, [] {});
    tab("Settings", GalleryTab::Settings,
        [&assets, &state] { DrawSettingsGallery(assets, state); });
    tab("Operation strip & tray", GalleryTab::Operations,
        [&assets, &state] { DrawOperationStateGallery(assets, state); });
    tab("Status bar", GalleryTab::Status,
        [&assets, &state] { DrawStatusBarStateGallery(assets, state); });
    ImGui::EndTabBar();
  }
  ImGui::End();

  if (state.active_tab == GalleryTab::Settings &&
      (state.settings.window_open ||
       state.settings.discard_confirmation_open)) {
    DrawSettingsGalleryWindow(assets, state);
  }
}

} // namespace fancy_ui::gallery
