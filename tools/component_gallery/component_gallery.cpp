#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace fancy_ui::gallery {

namespace {

void Heading(const char *title, ImFont *font) {
  if (font != nullptr) {
    ImGui::PushFont(font);
  }
  ImGui::TextUnformatted(title);
  if (font != nullptr) {
    ImGui::PopFont();
  }
  ImGui::Separator();
  ImGui::Spacing();
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
  if (ImGui::BeginChild("##card", ImVec2(Scale(300.0f), Scale(236.0f)),
                        ImGuiChildFlags_Borders,
                        scrollable ? ImGuiWindowFlags_NoSavedSettings
                                   : ImGuiWindowFlags_NoScrollbar)) {
    Heading(title, heading_font);
    draw();
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::PopID();
}

ButtonResult PreviewButton(const char *id, const char *label,
                           const detail::InteractionPreview preview) {
  const detail::ScopedInteractionPreview state(preview);
  return Button({
      .id = id,
      .label = label,
      .size = {.x = 64.0f, .y = 32.0f},
  });
}

void DrawButtons(GalleryState &state) {
  if (PreviewButton("default", "Default", detail::InteractionPreview::Rest)
          .activated) {
    state.button_feedback = "Default button activated.";
  }
  ImGui::SameLine();
  if (PreviewButton("hovered", "Hovered", detail::InteractionPreview::Hovered)
          .activated) {
    state.button_feedback = "Hovered preview activated.";
  }
  ImGui::SameLine();
  if (PreviewButton("pressed", "Pressed", detail::InteractionPreview::Pressed)
          .activated) {
    state.button_feedback = "Pressed preview activated.";
  }
  ImGui::SameLine();
  if (PreviewButton("focused", "Focused", detail::InteractionPreview::Focused)
          .activated) {
    state.button_feedback = "Focused preview activated.";
  }
  ImGui::Spacing();
  ImGui::TextDisabled("%s", state.button_feedback.c_str());
}

void DrawAvailability(GalleryState &state) {
  const ButtonResult selected = Button({
      .id = "selected",
      .label = "Selected",
      .selected = state.availability_selected,
      .size = {.x = 82.0f, .y = 32.0f},
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
      .size = {.x = 82.0f, .y = 32.0f},
  }));
  ImGui::SameLine();
  const ButtonResult invalid = Button({
      .id = "invalid",
      .label = "Invalid",
      .validation =
          {
              .invalid = true,
          },
      .size = {.x = 76.0f, .y = 32.0f},
  });
  ImGui::Spacing();
  ImGui::TextDisabled(
      "%s", invalid.activated ? "Invalid control activated; validation remains."
                              : "Disabled - select an eligible object");
}

void DrawInputs(GalleryState &state) {
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
  static_cast<void>(Checkbox({
      .id = "unavailable",
      .label = "Unavailable",
      .state = ToggleState::On,
      .availability =
          {
              .enabled = false,
              .reason = "Controlled by the active profile",
          },
  }));
}

void DrawVisibility(detail::UiAssetAtlas &assets, GalleryState &state) {
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
  const IconPainter enabled = assets.Painter("check");
  const IconPainter locked = assets.Painter("orbit-locked");
  const IconPainter unlocked = assets.Painter("orbit-unlocked");
  const CheckboxResult enabled_result = Checkbox({
      .id = "enabled",
      .label = "Direction enabled",
      .state = state.enabled,
      .on_icon = enabled,
  });
  if (enabled_result.changed) {
    state.enabled = enabled_result.state;
  }
  const CheckboxResult locked_result = Checkbox({
      .id = "locked",
      .label = state.locked == ToggleState::On ? "Direction locked"
                                               : "Direction unlocked",
      .state = state.locked,
      .on_icon = locked,
      .off_icon = unlocked,
  });
  if (locked_result.changed) {
    state.locked = locked_result.state;
  }
  static_cast<void>(Checkbox({
      .id = "inherited",
      .label = "Inherited and locked",
      .state = ToggleState::On,
      .on_icon = locked,
      .off_icon = unlocked,
      .availability =
          {
              .enabled = false,
              .reason = "Inherited from parent",
          },
  }));
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

void DrawTreeRows(detail::UiAssetAtlas &assets, GalleryState &state) {
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
  struct IssueDefinition {
    std::string_view id;
    std::string_view label;
    std::string_view detail;
    SemanticStatus status;
    std::string_view icon;
  };
  struct GroupDefinition {
    std::string_view id;
    std::string_view label;
    SemanticStatus status;
    std::string_view icon;
    std::size_t first_issue;
    std::size_t issue_count;
  };
  static constexpr std::array issues{
      IssueDefinition{"orphan-hole", "Orphan hole",
                      "Path 184 · invalid geometry", SemanticStatus::Failure,
                      "failure"},
      IssueDefinition{"self-intersection", "Self intersection",
                      "Path 211 · invalid geometry", SemanticStatus::Failure,
                      "failure"},
      IssueDefinition{"reversed-loop", "Reversed loop",
                      "Path 92 · repair available", SemanticStatus::Information,
                      "information"},
      IssueDefinition{"open-contour", "Open contour",
                      "Path 307 · review endpoint", SemanticStatus::Warning,
                      "alert"},
      IssueDefinition{"thin-feature", "Thin feature",
                      "Path 418 · below tool width", SemanticStatus::Warning,
                      "alert"},
  };
  static constexpr std::array groups{
      GroupDefinition{"invalid", "Invalid", SemanticStatus::Failure, "failure",
                      0, 2},
      GroupDefinition{"repairable", "Repairable", SemanticStatus::Information,
                      "information", 2, 1},
      GroupDefinition{"warnings", "Warnings", SemanticStatus::Warning, "alert",
                      3, 2},
  };

  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");
  const IconPainter review = assets.Painter("focus");

  const CheckboxResult labels = Checkbox({
      .id = "show-labels",
      .label = "Show issue labels in View",
      .state = state.show_issue_labels ? ToggleState::On : ToggleState::Off,
  });
  if (labels.changed) {
    state.show_issue_labels = labels.state == ToggleState::On;
    state.issue_feedback = state.show_issue_labels
                               ? "Issue labels shown in View."
                               : "Issue labels hidden in View.";
  }

  {
    HierarchyTree tree;
    for (std::size_t group_index = 0; group_index < groups.size();
         ++group_index) {
      const GroupDefinition &group = groups[group_index];
      const std::span<const ToggleState> descendants(
          state.issue_visibility.data() + group.first_issue, group.issue_count);
      const ToggleState group_visibility = AggregateVisibility(descendants);
      const std::string count = std::to_string(group.issue_count) +
                                (group.issue_count == 1 ? " issue" : " issues");
      const HierarchyRowResult group_result = HierarchyRow(
          tree,
          {
              .id = group.id,
              .label = group.label,
              .secondary_label = count,
              .expandable = true,
              .expanded = state.issue_groups_expanded[group_index],
              .status = group.status,
              .leading_icon = assets.Painter(group.icon),
              .visibility = group_visibility,
              .visible_icon = visible,
              .hidden_icon = hidden,
              .visibility_tooltip = "Show or hide every issue in this group",
          });
      if (group_result.expansion_changed) {
        state.issue_groups_expanded[group_index] = group_result.expanded;
      }
      if (group_result.visibility_changed) {
        std::fill_n(state.issue_visibility.begin() +
                        static_cast<std::ptrdiff_t>(group.first_issue),
                    group.issue_count, group_result.visibility);
        state.issue_feedback = std::string(group.label) +
                               (group_result.visibility == ToggleState::On
                                    ? " issues are visible."
                                    : " issues are hidden.");
      }

      if (!group_result.expanded) {
        continue;
      }
      for (std::size_t offset = 0; offset < group.issue_count; ++offset) {
        const std::size_t issue_index = group.first_issue + offset;
        const IssueDefinition &issue = issues[issue_index];
        const HierarchyRowResult issue_result = HierarchyRow(
            tree, {
                      .id = issue.id,
                      .label = issue.label,
                      .secondary_label = issue.detail,
                      .selected = state.issue_selected[issue_index],
                      .status = issue.status,
                      .leading_icon = assets.Painter(issue.icon),
                      .action_icon = review,
                      .action_tooltip = "Review issue in View",
                      .visibility = state.issue_visibility[issue_index],
                      .visible_icon = visible,
                      .hidden_icon = hidden,
                      .visibility_tooltip = "Show or hide this issue marker",
                  });
        if (issue_result.activated) {
          ApplySelection(state.issue_selected, state.issue_selection_anchor,
                         static_cast<int>(issue_index), issue_result.additive,
                         issue_result.range);
          state.issue_feedback = "Selected: " + std::string(issue.label) +
                                 " · " + std::string(issue.detail);
        }
        if (issue_result.visibility_changed) {
          state.issue_visibility[issue_index] = issue_result.visibility;
          state.issue_feedback = std::string(issue.label) +
                                 (issue_result.visibility == ToggleState::On
                                      ? " marker is visible."
                                      : " marker is hidden.");
        }
        if (issue_result.action_activated) {
          state.issue_selected.fill(false);
          state.issue_selected[issue_index] = true;
          state.issue_selection_anchor = static_cast<int>(issue_index);
          state.issue_feedback =
              "Review requested in View: " + std::string(issue.label);
        }
      }
      tree.Pop();
    }
  }
  ImGui::Spacing();
  ImGui::TextDisabled("Issue details");
  ImGui::TextWrapped("%s", state.issue_feedback.c_str());
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

void DrawMixedValues(detail::UiAssetAtlas &assets, GalleryState &state) {
  static_cast<void>(ValueDisplay({
      .id = "margin",
      .label = "Margin",
      .value = state.bed_color_mixed ? std::string_view{} : "Single value",
      .mixed = state.bed_color_mixed,
  }));
  ImGui::Spacing();
  static constexpr std::array colors{
      ColorRgba{.red = 0.27f, .green = 0.58f, .blue = 0.97f},
      ColorRgba{.red = 0.64f, .green = 0.44f, .blue = 0.97f},
  };
  const std::span<const ColorRgba> preview =
      state.bed_color_mixed ? std::span<const ColorRgba>(colors)
                            : std::span<const ColorRgba>(&state.bed_color, 1);
  const ColorSwatchResult color = ColorSwatch(
      {
          .id = "color",
          .label = "Bed color",
          .tooltip = "Open color picker",
          .picker_title = "Bed color",
          .value = state.bed_color,
          .colors = preview,
      },
      state.bed_color_picker);
  if (color.changed) {
    state.bed_color = color.value;
    state.bed_color_mixed = false;
  }
  const CheckboxResult margins = Checkbox({
      .id = "checkbox",
      .label = "Margins - Mixed",
      .state = state.margins,
  });
  if (margins.changed) {
    state.margins = margins.state;
  }
  const VisibilityToggleResult visibility = VisibilityToggle({
      .id = "visibility",
      .label = "Visibility",
      .state = state.mixed_visibility,
      .visible_icon = assets.Painter("visibility"),
      .hidden_icon = assets.Painter("visibility-off"),
  });
  if (visibility.changed) {
    state.mixed_visibility = visibility.state;
  }
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

void DrawComponentGallery(detail::UiAssetAtlas &assets, GalleryState &state) {
  ApplyTheme(state.theme, state.scale);
  assets.InstallPendingIcons();

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::Begin("Fancy UI component gallery", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  if (assets.bold_font() != nullptr) {
    ImGui::PushFont(assets.bold_font());
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
  }
  ImGui::TextDisabled(
      "Canonical light/dark parity; pointer and keyboard states remain live.");
  ImGui::Spacing();

  const auto move_tab = [&state](const int delta) {
    constexpr int tab_count = 3;
    const int current = static_cast<int>(state.active_tab);
    state.active_tab =
        static_cast<GalleryTab>((current + delta + tab_count) % tab_count);
  };
  if (ImGui::BeginTabBar("##gallery-tabs",
                         ImGuiTabBarFlags_FittingPolicyResizeDown)) {
    const auto tab = [&](const char *label, const GalleryTab gallery_tab,
                         const std::function<void()> &draw) {
      const ImGuiTabItemFlags flags = state.active_tab == gallery_tab
                                          ? ImGuiTabItemFlags_SetSelected
                                          : ImGuiTabItemFlags_None;
      const bool visible = ImGui::BeginTabItem(label, nullptr, flags);
      const bool focused = ImGui::IsItemFocused();
      if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        state.active_tab = gallery_tab;
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
      ImGui::TextDisabled(
          "Shared controls, hierarchy rows, semantic feedback, and values.");
      ImGui::Spacing();
      const float table_width = Scale(4.0f * 300.0f + 3.0f * 8.0f);
      if (ImGui::BeginChild("##gallery-scroll", ImVec2(0.0f, 0.0f), false,
                            ImGuiWindowFlags_HorizontalScrollbar)) {
        if (ImGui::BeginTable("##component-grid", 4,
                              ImGuiTableFlags_SizingFixedFit,
                              ImVec2(table_width, 0.0f))) {
          for (int column = 0; column < 4; ++column) {
            ImGui::TableSetupColumn(
                "component", ImGuiTableColumnFlags_WidthFixed, Scale(300.0f));
          }
          GalleryCard("buttons", "Buttons", assets.bold_font(),
                      [&state] { DrawButtons(state); });
          GalleryCard("availability", "Availability", assets.bold_font(),
                      [&state] { DrawAvailability(state); });
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
          GalleryCard("mixed-values", "Mixed values", assets.bold_font(),
                      [&assets, &state] { DrawMixedValues(assets, state); });
          GalleryCard("empty-overflow", "Empty & overflow", assets.bold_font(),
                      DrawEmptyOverflow);
          GalleryCard("color-pickers", "Color pickers", assets.bold_font(),
                      [&state] { DrawColorPickers(state); });
          ImGui::EndTable();
        }
      }
      ImGui::EndChild();
    });
    tab("Operation strip & tray", GalleryTab::Operations,
        [&assets, &state] { DrawOperationStateGallery(assets, state); });
    tab("Status bar", GalleryTab::Status,
        [&assets, &state] { DrawStatusBarStateGallery(assets, state); });
    ImGui::EndTabBar();
  }
  ImGui::End();
}

} // namespace fancy_ui::gallery
