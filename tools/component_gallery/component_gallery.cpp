#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/application_chrome.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <format>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace fancy_ui::gallery {

namespace {

FontHandle NativeFontHandle(ImFont *font);

void Heading(const char *title, ImFont *font) {
  if (font != nullptr) {
    ImGui::PushFont(
        font, CurrentLayoutMetrics().typography.section_heading_font_height);
  }
  ImGui::TextUnformatted(title);
  if (font != nullptr) {
    ImGui::PopFont();
  }
}

void GalleryCard(const char *id, const char *title, ImFont *heading_font,
                 const std::function<void()> &draw,
                 const bool scrollable = false, const bool wide = false,
                 const float logical_height = 220.0f,
                 const float logical_width = 0.0f) {
  ImGui::TableNextColumn();
  ImGui::PushID(id);
  ImGui::PushStyleColor(
      ImGuiCol_ChildBg,
      ImVec4(CurrentPalette().surface.red, CurrentPalette().surface.green,
             CurrentPalette().surface.blue, CurrentPalette().surface.alpha));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(Scale(12.0f), Scale(8.0f)));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
  const float default_width = wide ? 2.0f * 302.0f + 8.0f : 302.0f;
  const float width =
      Scale(logical_width > 0.0f ? logical_width : default_width);
  if (ImGui::BeginChild("##card", ImVec2(width, Scale(logical_height)),
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
  if (wide) {
    ImGui::TableNextColumn();
  }
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
  detail::DrawSecondaryText(
      invalid.activated ? "Invalid control activated; validation remains."
                        : "Disabled · select an eligible object");
}

void DrawDisclosureRows(GalleryState &state, ImFont *heading_font) {
  const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing.x, 0.0f));
  const DisclosureRowResult live = DisclosureRow({
      .id = "live-header",
      .label = "Model operations",
      .metadata = "Live",
      .variant = DisclosureRowVariant::PanelHeader,
      .expandable = true,
      .expanded = state.disclosure_row_open,
      .font = NativeFontHandle(heading_font),
  });
  if (live.expansion_changed) {
    state.disclosure_row_open = live.expanded;
  } else if (live.activated) {
    state.disclosure_row_open = !state.disclosure_row_open;
  }
  {
    const detail::ScopedInteractionPreview preview(
        detail::InteractionPreview::Hovered);
    static_cast<void>(DisclosureRow({
        .id = "hovered-header",
        .label = "Hovered panel header",
        .variant = DisclosureRowVariant::PanelHeader,
        .expandable = true,
        .expanded = true,
        .font = NativeFontHandle(heading_font),
    }));
  }
  {
    const detail::ScopedInteractionPreview preview(
        detail::InteractionPreview::Pressed);
    static_cast<void>(DisclosureRow({
        .id = "pressed-header",
        .label = "Pressed panel header",
        .variant = DisclosureRowVariant::PanelHeader,
        .expandable = true,
        .font = NativeFontHandle(heading_font),
    }));
  }
  {
    const detail::ScopedInteractionPreview preview(
        detail::InteractionPreview::Focused);
    static_cast<void>(DisclosureRow({
        .id = "focused-item",
        .label = "Focused hierarchy item",
        .expandable = true,
        .expanded = true,
    }));
  }
  static_cast<void>(DisclosureRow({
      .id = "selected-item",
      .label = "Selected hierarchy item",
      .selected = true,
  }));
  static_cast<void>(DisclosureRow({
      .id = "action-item",
      .label = "Hierarchy item with action",
      .reserved_trailing_width = 72.0f,
  }));
  const ImVec2 row_minimum = ImGui::GetItemRectMin();
  const ImVec2 row_maximum = ImGui::GetItemRectMax();
  const ImVec2 cursor_after_row = ImGui::GetCursorScreenPos();
  ImGui::SetCursorScreenPos(
      ImVec2(row_maximum.x - Scale(68.0f), row_minimum.y + Scale(4.0f)));
  static_cast<void>(Button({
      .id = "row-action",
      .label = "Action",
      .size = {.x = 64.0f, .y = 24.0f},
  }));
  ImGui::SetCursorScreenPos(cursor_after_row);
  static_cast<void>(DisclosureRow({
      .id = "disabled-item",
      .label = "Disabled hierarchy item",
      .availability = {.enabled = false, .reason = "Unavailable example"},
  }));
  ImGui::PopStyleVar();
}

void DrawWorkspaceSwitcher(detail::ApplicationChrome &chrome,
                           GalleryState &state) {
  using steppenface::WorkspaceKind;
  const bool model = state.component_workspace == WorkspaceKind::Model3d;
  detail::DrawSecondaryText(model ? "3D · selected" : "Canvas · selected");
  detail::DrawSecondaryText(model ? "Canvas · rest" : "3D · rest");
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
  detail::DrawSecondaryText(canvas ? "Canvas · selected" : "Object · selected");
  detail::DrawSecondaryText(canvas ? "Object · hover" : "Canvas · hover");
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
  detail::DrawSecondaryText("Pointer · selected");
  detail::DrawSecondaryText("Rectangle · pressed · Oval · disabled");
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
    detail::DrawSecondaryText("0%");
    ImGui::TableNextColumn();
    detail::DrawSecondaryText("Exact value");
    ImGui::TableNextColumn();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                         ImGui::GetContentRegionAvail().x -
                         ImGui::CalcTextSize("100%").x);
    detail::DrawSecondaryText("100%");
    ImGui::EndTable();
  }
}

void DrawCompass(GalleryState &state, const bool inherited) {
  const RotationCompassResult result = RotationCompass({
      .id = inherited ? "inherited" : "local",
      .label = "Rotations",
      .count = state.rotations,
      .inherited = inherited,
      .availability =
          inherited ? Availability{.enabled = false,
                                   .reason = "Enable Rotation override to "
                                             "change search rotations."}
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
  detail::DrawSecondaryTextWrapped(unavailable_reason);
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
    ImGui::PushFont(nullptr,
                    CurrentLayoutMetrics().typography.body_font_height);
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

void DrawNavigationPrimitives(GalleryState &state) {
  static constexpr std::array choices{
      ChoiceSpec{.id = "canvas", .label = "Canvas"},
      ChoiceSpec{.id = "model", .label = "3D"},
  };
  const SegmentedControlResult segmented = SegmentedControl({
      .id = "workspace",
      .choices = choices,
      .selected_index = state.segmented_index,
      .width = 276.0f,
  });
  if (segmented.changed) {
    state.segmented_index = segmented.selected_index;
  }

  const TabSetResult tabs = TabSet({
      .id = "tabs",
      .tabs = choices,
      .selected_index = state.tab_index,
      .draw_panel =
          [](const std::size_t index) {
            detail::DrawSecondaryText(index == 0 ? "Canvas panel" : "3D panel");
          },
  });
  if (tabs.changed) {
    state.tab_index = tabs.selected_index;
  }

  const ExplorerSearchResult search = ExplorerSearch({
      .id = "search",
      .placeholder = "Search objects",
      .query = state.explorer_query,
  });
  if (search.changed) {
    state.explorer_query = search.query;
  }

  static const std::array arrange_items{
      ContextMenuItemSpec{.id = "front", .label = "Bring to front"},
      ContextMenuItemSpec{.id = "back", .label = "Send to back"},
  };
  static const std::array menu_items{
      ContextMenuItemSpec{.id = "rename", .label = "Rename"},
      ContextMenuItemSpec{.id = "separator",
                          .kind = ContextMenuItemKind::Separator},
      ContextMenuItemSpec{.id = "arrange",
                          .label = "Arrange",
                          .kind = ContextMenuItemKind::Submenu,
                          .children = arrange_items},
  };
  const ButtonResult menu_trigger = Button({
      .id = "menu-trigger",
      .label = "Context menu",
      .variant = ButtonVariant::Secondary,
      .size = {.x = 132.0f, .y = 28.0f},
  });
  const bool request_menu =
      menu_trigger.activated ||
      state.component_capture == ComponentCaptureVariant::ContextMenuOpen;
  const ContextMenuResult menu = ContextMenu(
      {
          .id = "menu",
          .items = menu_items,
          .request_open = request_menu,
          .anchor = request_menu ? std::optional<Vec2>{Vec2{
                                       .x = ImGui::GetItemRectMin().x,
                                       .y = ImGui::GetItemRectMax().y,
                                   }}
                                 : std::nullopt,
      },
      state.context_menu);
  if (menu.activated_id.has_value()) {
    state.explorer_query = *menu.activated_id;
  }
  ImGui::SameLine();
  const StatusZoomPopoverResult zoom = StatusZoomPopover(
      {
          .id = "zoom",
          .percent = state.zoom_percent,
          .request_open = state.component_capture ==
                          ComponentCaptureVariant::StatusZoomOpen,
          .selection_availability =
              {
                  .enabled = false,
                  .reason = "Select an object to zoom to it",
              },
      },
      state.zoom_popover);
  if (zoom.changed) {
    state.zoom_percent = zoom.percent;
  } else if (zoom.command == StatusZoomCommand::ActualSize) {
    state.zoom_percent = 100.0f;
  }
}

void DrawAdvancedFields(GalleryState &state) {
  const detail::ScopedFieldLayoutPreview layout(Scale(76.0f));
  const std::array options{
      SelectOption{.id = "draft", .label = state.renamable_labels[0]},
      SelectOption{.id = "production", .label = state.renamable_labels[1]},
      SelectOption{.id = "archive", .label = state.renamable_labels[2]},
  };
  const RenamableSelectResult renamable = RenamableSelect(
      {
          .id = "profile",
          .label = "Profile",
          .options = options,
          .selected_index = state.renamable_index,
      },
      state.renamable_select);
  if (renamable.selection_changed) {
    state.renamable_index = renamable.selected_index;
  }
  if (renamable.committed) {
    state.renamable_labels[state.renamable_index] = renamable.value;
  }

  const std::array multiselect_options{
      CheckedMultiselectOption{.id = "guides",
                               .label = "Guides",
                               .state = state.multiselect_states[0]},
      CheckedMultiselectOption{.id = "major-grid",
                               .label = "Major grid",
                               .state = state.multiselect_states[1]},
      CheckedMultiselectOption{.id = "minor-grid",
                               .label = "Minor grid",
                               .state = state.multiselect_states[2]},
  };
  const auto enabled_count = static_cast<int>(std::ranges::count_if(
      state.multiselect_states,
      [](const ToggleState value) { return value != ToggleState::Off; }));
  const std::string summary = std::format("{} of 3", enabled_count);
  const CheckedMultiselectResult multiselect = CheckedMultiselect({
      .id = "snapping",
      .label = "Snapping",
      .summary = summary,
      .options = multiselect_options,
      .request_open =
          state.component_capture == ComponentCaptureVariant::MultiselectOpen,
  });
  if (multiselect.changed && multiselect.option_id.has_value()) {
    for (std::size_t index = 0; index < multiselect_options.size(); ++index) {
      if (multiselect_options[index].id == *multiselect.option_id) {
        state.multiselect_states[index] = multiselect.state;
      }
    }
  }

  static constexpr std::array radio_options{
      SelectOption{.id = "top-left", .label = "Top left"},
      SelectOption{.id = "center", .label = "Center"},
  };
  const RadioGroupResult radio = RadioGroup({
      .id = "origin",
      .label = "Origin",
      .options = radio_options,
      .selected_index = state.radio_index,
      .layout = RadioGroupLayout::Horizontal,
  });
  if (radio.changed) {
    state.radio_index = radio.selected_index;
  }

  static constexpr std::array metrics{
      MetricValue{.label = "Placed", .value = "19 / 22"},
      MetricValue{.label = "Use", .value = "87.4%"},
  };
  MetricRow({.id = "metrics", .label = "Current best", .metrics = metrics});
}

void DrawSpatialGizmo(detail::UiAssetAtlas &assets, GalleryState &state) {
  const bool bed = state.component_capture == ComponentCaptureVariant::GrainBed;
  const bool part =
      state.component_capture == ComponentCaptureVariant::GrainPart;
  const std::optional<GrainDirectionValue> secondary =
      bed || part ? std::nullopt
                  : std::optional{GrainDirectionValue{
                        .kind = GrainDirectionKind::Bed, .degrees = 90.0}};
  const GrainDirectionGizmoResult result = GrainDirectionGizmo(
      {
          .id = "grain",
          .primary = {.kind = bed ? GrainDirectionKind::Bed
                                  : GrainDirectionKind::Part,
                      .degrees = state.grain_degrees},
          .secondary = secondary,
          .locked =
              state.component_capture == ComponentCaptureVariant::GrainLocked,
          .regular_font = NativeFontHandle(assets.font(UiFontWeight::Regular)),
          .medium_font = NativeFontHandle(assets.font(UiFontWeight::Medium)),
          .bold_font = NativeFontHandle(assets.font(UiFontWeight::Bold)),
          .monospace_font = NativeFontHandle(assets.mono_font()),
      },
      state.grain_gizmo);
  if (result.committed) {
    state.grain_degrees = result.degrees;
  }
}

void DrawSpatialOverlay() {
  static constexpr std::array points{
      Vec2{.x = 4.0f, .y = 4.0f},
      Vec2{.x = 250.0f, .y = 20.0f},
      Vec2{.x = 220.0f, .y = 124.0f},
      Vec2{.x = 24.0f, .y = 110.0f},
  };
  SpatialOverlay({
      .id = "selection-overlay",
      .points = points,
      .label = "Auto-split preview",
      .status = SemanticStatus::Preview,
      .pattern = SpatialOverlayPattern::Hatched,
      .selected = true,
  });
}

void DrawLayoutAndFeedback(GalleryState &state) {
  const OperationDisclosureResult disclosure = OperationDisclosure({
      .id = "disclosure",
      .expanded = state.operation_expanded,
  });
  if (disclosure.changed) {
    state.operation_expanded = disclosure.expanded;
  }
  ImGui::SameLine();
  detail::DrawSecondaryText(state.operation_expanded ? "Details expanded"
                                                     : "Details collapsed");
  const ResizeHandleResult resize = ResizeHandle({
      .id = "resize",
      .value = state.resize_value,
      .minimum = 160.0f,
      .maximum = 240.0f,
      .keyboard_step = 8.0f,
      .reset_value = 200.0f,
      .direction = ResizeDirection::Vertical,
      .tooltip = "Resize operation details",
  });
  if (resize.changed) {
    state.resize_value = resize.value;
  }
  detail::DrawSecondaryText(
      std::format("Tray height · {:.0f} px", state.resize_value));
  const ButtonResult open = Button({
      .id = "open-window",
      .label = "Open modeless window",
      .size = {.x = 188.0f, .y = 28.0f},
  });
  const ModelessWindowResult window = ModelessWindow(
      {
          .id = "component-modeless",
          .title = "File task",
          .request_open =
              open.activated || state.component_capture ==
                                    ComponentCaptureVariant::ModelessWindowOpen,
          .initial_size = {.x = 420.0f, .y = 240.0f},
          .draw_content =
              [] {
                StatusCard({
                    .id = "task-status",
                    .message = "Prepared UI content remains interactive",
                    .status = SemanticStatus::Information,
                });
              },
      },
      state.modeless_window);
  state.modeless_window = window.state;
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

FontHandle NativeFontHandle(ImFont *font) {
  return FontHandle{.value = reinterpret_cast<std::uintptr_t>(font)};
}

struct HierarchyFixtureRow {
  std::string_view id;
  std::string_view label;
  std::string_view metadata;
  std::string_view icon;
  int depth = 0;
  SemanticStatus status = SemanticStatus::Neutral;
};

constexpr std::array<HierarchyFixtureRow, 7> kStepHierarchy{{
    {.id = "assembly",
     .label = "Assembly.step",
     .metadata = "2 parts",
     .icon = "file"},
    {.id = "part-001",
     .label = "Part_001.step",
     .metadata = "3 bodies",
     .icon = "model",
     .depth = 1},
    {.id = "body-1",
     .label = "Body_1",
     .metadata = "Solid",
     .icon = "model",
     .depth = 2,
     .status = SemanticStatus::Success},
    {.id = "body-2",
     .label = "Body_2",
     .metadata = "Solid",
     .icon = "model",
     .depth = 2},
    {.id = "body-3",
     .label = "Body_3",
     .metadata = "Solid",
     .icon = "model",
     .depth = 2,
     .status = SemanticStatus::Warning},
    {.id = "bracket",
     .label = "Bracket.step",
     .metadata = "1 body",
     .icon = "model",
     .depth = 1},
    {.id = "bracket-body",
     .label = "Body_1",
     .metadata = "Solid",
     .icon = "model",
     .depth = 2,
     .status = SemanticStatus::Success},
}};

constexpr std::array<HierarchyFixtureRow, 7> kSvgHierarchy{{
    {.id = "drawing",
     .label = "Drawing.svg",
     .metadata = "2 layers",
     .icon = "svg"},
    {.id = "layer-1",
     .label = "Layer_1",
     .metadata = "2 paths",
     .icon = "objects",
     .depth = 1},
    {.id = "group-3",
     .label = "Group 3",
     .metadata = "2 paths",
     .icon = "folder",
     .depth = 2},
    {.id = "path-184",
     .label = "Path 184",
     .metadata = "Closed",
     .icon = "path",
     .depth = 3,
     .status = SemanticStatus::Success},
    {.id = "path-185",
     .label = "Path 185",
     .metadata = "Open",
     .icon = "path",
     .depth = 3,
     .status = SemanticStatus::Warning},
    {.id = "layer-2",
     .label = "Labels",
     .metadata = "1 text",
     .icon = "objects",
     .depth = 1},
    {.id = "title-text",
     .label = "Title text",
     .metadata = "Text",
     .icon = "typography",
     .depth = 2},
}};

constexpr std::array<HierarchyFixtureRow, 7> kDxfHierarchy{{
    {.id = "cutout",
     .label = "Cutout.dxf",
     .metadata = "2 layers",
     .icon = "dxf"},
    {.id = "cut-layer",
     .label = "CUT",
     .metadata = "3 entities",
     .icon = "objects",
     .depth = 1},
    {.id = "entities",
     .label = "Entities",
     .metadata = "Geometry",
     .icon = "folder",
     .depth = 2},
    {.id = "line-12",
     .label = "Line 12",
     .metadata = "42.0 mm",
     .icon = "line",
     .depth = 3,
     .status = SemanticStatus::Success},
    {.id = "arc-4",
     .label = "Arc 4",
     .metadata = "R 18.0 mm",
     .icon = "arc",
     .depth = 3,
     .status = SemanticStatus::Warning},
    {.id = "circle-2",
     .label = "Circle 2",
     .metadata = "Ø 8.0 mm",
     .icon = "circle",
     .depth = 3},
    {.id = "layer-0",
     .label = "Layer 0",
     .metadata = "Default",
     .icon = "objects",
     .depth = 1},
}};

std::size_t SubtreeEnd(const std::span<const HierarchyFixtureRow> rows,
                       const std::size_t index) {
  std::size_t end = index + 1;
  while (end < rows.size() && rows[end].depth > rows[index].depth) {
    ++end;
  }
  return end;
}

ToggleState HierarchyVisibility(const HierarchyCardState &state,
                                const std::size_t first,
                                const std::size_t last) {
  ToggleState visibility = state.visibility[first];
  for (std::size_t index = first + 1; index < last; ++index) {
    if (state.visibility[index] != visibility) {
      return ToggleState::Mixed;
    }
  }
  return visibility;
}

void DrawHierarchyFixtureNode(detail::UiAssetAtlas &assets,
                              HierarchyCardState &state,
                              const std::span<const HierarchyFixtureRow> rows,
                              const std::size_t index, HierarchyTree &tree,
                              bool &request_color_picker) {
  const HierarchyFixtureRow &row = rows[index];
  const std::size_t subtree_end = SubtreeEnd(rows, index);
  const bool expandable = subtree_end > index + 1;
  const ToggleState visibility = HierarchyVisibility(state, index, subtree_end);
  const bool request_color_focus = state.color_picker.restore_focus &&
                                   state.color_row == static_cast<int>(index);
  const HierarchyRowResult result =
      HierarchyRow(tree, {
                             .id = row.id,
                             .label = row.label,
                             .metadata = row.metadata,
                             .expandable = expandable,
                             .expanded = expandable && state.expanded[index],
                             .selected = state.selected[index],
                             .status = row.status,
                             .leading_icon = assets.Painter(row.icon),
                             .color = state.colors[index],
                             .color_tooltip = "Edit row color",
                             .request_color_focus = request_color_focus,
                             .action_icon = assets.Painter("more"),
                             .action_tooltip = "Row actions",
                             .visibility = visibility,
                             .visible_icon = assets.Painter("visibility"),
                             .hidden_icon = assets.Painter("visibility-off"),
                             .visibility_tooltip = "Show or hide row",
                         });
  if (request_color_focus) {
    state.color_picker.restore_focus = false;
  }
  if (result.expansion_changed) {
    state.expanded[index] = result.expanded;
  }
  if (result.activated) {
    ApplySelection(state.selected, state.selection_anchor,
                   static_cast<int>(index), result.additive, result.range);
  }
  if (result.visibility_changed) {
    for (std::size_t descendant = index; descendant < subtree_end;
         ++descendant) {
      state.visibility[descendant] = result.visibility;
    }
  }
  if (result.color_activated) {
    state.color_row = static_cast<int>(index);
    request_color_picker = true;
  }
  if (result.action_activated) {
    state.action_row = static_cast<int>(index);
    ImGui::OpenPopup("##hierarchy-actions");
  }

  if (!expandable || !result.expanded) {
    return;
  }
  std::size_t child = index + 1;
  while (child < subtree_end) {
    DrawHierarchyFixtureNode(assets, state, rows, child, tree,
                             request_color_picker);
    child = SubtreeEnd(rows, child);
  }
  tree.Pop();
}

void DrawCanonicalHierarchy(detail::UiAssetAtlas &assets,
                            HierarchyCardState &state,
                            const std::span<const HierarchyFixtureRow> rows,
                            const std::string_view label) {
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  ImGui::TextColored(ImVec4(CurrentPalette().text_secondary.red,
                            CurrentPalette().text_secondary.green,
                            CurrentPalette().text_secondary.blue,
                            CurrentPalette().text_secondary.alpha),
                     "%.*s HIERARCHY", static_cast<int>(label.size()),
                     label.data());
  ImGui::PopFont();

  bool request_color_picker = false;
  {
    HierarchyTree tree(
        {.section_font = NativeFontHandle(assets.heading_font())});
    std::size_t root = 0;
    while (root < rows.size()) {
      DrawHierarchyFixtureNode(assets, state, rows, root, tree,
                               request_color_picker);
      root = SubtreeEnd(rows, root);
    }
  }

  const int color_row =
      std::clamp(state.color_row, 0, static_cast<int>(rows.size()) - 1);
  const std::string picker_title =
      std::string(rows[static_cast<std::size_t>(color_row)].label) + " color";
  const ColorPickerPopupResult color = ColorPickerPopup(
      {
          .id = "hierarchy-color",
          .title = picker_title,
          .value = state.colors[static_cast<std::size_t>(color_row)],
          .request_open = request_color_picker,
      },
      state.color_picker);
  if (color.changed) {
    state.colors[static_cast<std::size_t>(color_row)] = color.value;
  }

  if (state.request_actions) {
    ImGui::OpenPopup("##hierarchy-actions");
    state.request_actions = false;
  }
  detail::PushMenuPopupStyle();
  if (ImGui::BeginPopup("##hierarchy-actions")) {
    const int action_row =
        std::clamp(state.action_row, 0, static_cast<int>(rows.size()) - 1);
    ImGui::TextUnformatted(
        std::string(rows[static_cast<std::size_t>(action_row)].label).c_str());
    ImGui::Separator();
    static_cast<void>(ImGui::MenuItem("Frame in view"));
    static_cast<void>(ImGui::MenuItem("Inspect properties"));
    ImGui::EndPopup();
  }
  detail::PopMenuPopupStyle();
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
    HierarchyTree tree(
        {.section_font = NativeFontHandle(assets.heading_font())});
    const HierarchyRowResult assembly = HierarchyRow(
        tree, {
                  .id = "assembly",
                  .label = labels[0],
                  .metadata = assembly_visibility == ToggleState::Mixed
                                  ? "Assembly · mixed visibility"
                                  : "Assembly · 2 descendants",
                  .expandable = true,
                  .expanded = state.assembly_expanded,
                  .selected = state.tree_selected[0],
                  .leading_icon = assets.Painter("file"),
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
                    .metadata = "Part 4 · 1 path",
                    .expandable = true,
                    .expanded = state.part_expanded,
                    .selected = state.tree_selected[1],
                    .leading_icon = assets.Painter("model"),
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
                      .metadata = "Path 184 · closed contour",
                      .selected = state.tree_selected[2],
                      .leading_icon = assets.Painter("path"),
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

  detail::PushMenuPopupStyle();
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
  detail::PopMenuPopupStyle();
  ImGui::Spacing();
  detail::DrawSecondaryText(state.tree_feedback);
}

void DrawIssueHierarchy(detail::UiAssetAtlas &assets, GalleryState &state) {
  const detail::ScopedFieldLayoutPreview layout(Scale(96.0f));
  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");

  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
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

  const ToggleState invalid_visibility =
      AggregateVisibility(state.invalid_issue_visibility);
  {
    InformationTree tree(
        {.section_font = NativeFontHandle(assets.heading_font())});
    const InformationTreeRowResult invalid = InformationTreeRow(
        tree, {
                  .id = "invalid",
                  .label = "Invalid",
                  .metadata = "1",
                  .expandable = true,
                  .expanded = state.invalid_issues_expanded,
                  .status = SemanticStatus::Failure,
                  .visibility = invalid_visibility,
                  .visible_icon = visible,
                  .hidden_icon = hidden,
                  .visibility_tooltip = "Show or hide all invalid issues",
              });
    if (invalid.expansion_changed) {
      state.invalid_issues_expanded = invalid.expanded;
    }
    if (invalid.visibility_changed) {
      state.invalid_issue_visibility.fill(invalid.visibility);
    }
    if (invalid.expanded) {
      const InformationTreeRowResult self_intersection = InformationTreeRow(
          tree, {
                    .id = "self-intersection",
                    .label = "Self-intersection",
                    .metadata = "1",
                    .status = SemanticStatus::Failure,
                    .visibility = state.invalid_issue_visibility[0],
                    .visible_icon = visible,
                    .hidden_icon = hidden,
                    .visibility_tooltip = "Show or hide self-intersections",
                });
      if (self_intersection.visibility_changed) {
        state.invalid_issue_visibility[0] = self_intersection.visibility;
      }
      tree.Pop();
    }
  }

  const ToggleState repairable_visibility =
      AggregateVisibility(state.issue_visibility);
  {
    InformationTree tree(
        {.section_font = NativeFontHandle(assets.heading_font())});
    const InformationTreeRowResult repairable = InformationTreeRow(
        tree, {
                  .id = "repairable",
                  .label = "Repairable",
                  .metadata = "5",
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
                    .metadata = "4",
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
                    .metadata = "1",
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

  const ToggleState warning_visibility =
      AggregateVisibility(state.warning_issue_visibility);
  {
    InformationTree tree(
        {.section_font = NativeFontHandle(assets.heading_font())});
    const InformationTreeRowResult warnings = InformationTreeRow(
        tree, {
                  .id = "warnings",
                  .label = "Warnings",
                  .metadata = "4",
                  .expandable = true,
                  .expanded = state.warning_issues_expanded,
                  .status = SemanticStatus::Warning,
                  .visibility = warning_visibility,
                  .visible_icon = visible,
                  .hidden_icon = hidden,
                  .visibility_tooltip = "Show or hide all warnings",
              });
    if (warnings.expansion_changed) {
      state.warning_issues_expanded = warnings.expanded;
    }
    if (warnings.visibility_changed) {
      state.warning_issue_visibility.fill(warnings.visibility);
    }
    if (warnings.expanded) {
      const InformationTreeRowResult ambiguous_cleanup = InformationTreeRow(
          tree, {
                    .id = "ambiguous-cleanup",
                    .label = "Ambiguous cleanup",
                    .metadata = "4",
                    .status = SemanticStatus::Warning,
                    .visibility = state.warning_issue_visibility[0],
                    .visible_icon = visible,
                    .hidden_icon = hidden,
                    .visibility_tooltip = "Show or hide ambiguous cleanup",
                });
      if (ambiguous_cleanup.visibility_changed) {
        state.warning_issue_visibility[0] = ambiguous_cleanup.visibility;
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
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  ImGui::TextUnformatted("Determinate · 62%");
  ImGui::PopFont();
  ProgressBar({
      .id = "determinate",
      .label = "Search progress: 62%",
      .value = 0.62f,
      .status = SemanticStatus::Busy,
  });
  ImGui::Spacing();
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  ImGui::TextUnformatted("Indeterminate");
  ImGui::PopFont();
  ProgressBar({
      .id = "indeterminate",
      .label = "Preparing geometry",
      .value = std::nullopt,
      .status = SemanticStatus::Busy,
  });
  detail::DrawSecondaryText("Preparing geometry...");
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
  const auto intrinsic_button_width = [font,
                                       &metrics](const std::string_view label) {
    return font->CalcTextSizeA(metrics.typography.body_font_height,
                               std::numeric_limits<float>::max(), 0.0f,
                               label.data(), label.data() + label.size())
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
  ApplyTheme(state.theme, assets.ui_environment());
  assets.InstallPendingIcons();

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  const bool shell_preview = IsFullCanvasPreview(state.active_tab);
  if (shell_preview) {
    const bool escape_owned_at_frame_start =
        ImGui::IsAnyItemActive() ||
        ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId |
                                   ImGuiPopupFlags_AnyPopupLevel);
    const bool return_requested = DrawApplicationShellGallery(assets, state);
    const bool escape_requested = ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    if (return_requested ||
        (escape_requested && !escape_owned_at_frame_start)) {
      LeaveShellPreview(state);
    }
    return;
  }
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(Scale(24.0f), Scale(24.0f)));
  ImGui::Begin("Fancy UI component gallery", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);
  ImGui::PopStyleVar();

  if (assets.heading_font() != nullptr) {
    ImGui::PushFont(assets.heading_font(),
                    CurrentLayoutMetrics().typography.page_title_font_height);
  }
  ImGui::TextUnformatted("Fancy UI gallery");
  if (assets.heading_font() != nullptr) {
    ImGui::PopFont();
  }
  ImGui::SameLine();
  detail::DrawSecondaryText(std::format(
      "- Fancy UI - {:.0f}%", assets.ui_environment().raster_scale * 100.0f));
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
  ImGui::PushFont(nullptr, CurrentLayoutMetrics().typography.body_font_height);
  detail::DrawSecondaryText(
      "Canonical light/dark parity; pointer and keyboard states remain live.");
  ImGui::PopFont();
  ImGui::Spacing();

  static constexpr std::array gallery_tabs{
      ChoiceSpec{.id = "components", .label = "Components"},
      ChoiceSpec{.id = "shell", .label = "Application shell"},
      ChoiceSpec{.id = "panel-audits", .label = "Panel audits"},
      ChoiceSpec{.id = "settings", .label = "Settings"},
      ChoiceSpec{.id = "operations", .label = "Operation strip & tray"},
      ChoiceSpec{.id = "status", .label = "Status bar"},
  };
  static_assert(gallery_tabs.size() == kGalleryTabCount);
  const TabSetResult selected_tab = TabSet({
      .id = "gallery-tabs",
      .tabs = gallery_tabs,
      .selected_index = static_cast<std::size_t>(state.active_tab),
      .request_focus = state.focus_active_tab,
  });
  const GalleryTab active_tab =
      static_cast<GalleryTab>(selected_tab.selected_index);
  if (active_tab != state.active_tab) {
    ActivateGalleryTab(state, active_tab);
  }
  state.focus_active_tab = false;
  {
    const auto panel = [&state](const GalleryTab tab, const auto &draw) {
      if (state.active_tab == tab) {
        draw();
      }
    };
    panel(GalleryTab::Components, [&assets, &state] {
      detail::ApplicationChrome chrome(assets);
      ImGui::PushFont(nullptr,
                      CurrentLayoutMetrics().typography.body_font_height);
      detail::DrawSecondaryText("Shared controls, hierarchy rows, "
                                "semantic feedback, and values.");
      ImGui::PopFont();
      ImGui::Spacing();
      const float table_width = Scale(4.0f * 302.0f + 3.0f * 8.0f);
      if (ImGui::BeginChild("##gallery-scroll", ImVec2(0.0f, 0.0f), false,
                            ImGuiWindowFlags_HorizontalScrollbar)) {
        if (ImGui::BeginTable("##component-grid", 4,
                              ImGuiTableFlags_SizingFixedFit |
                                  ImGuiTableFlags_NoClip,
                              ImVec2(table_width, 0.0f))) {
          for (int column = 0; column < 4; ++column) {
            ImGui::TableSetupColumn(
                "component", ImGuiTableColumnFlags_WidthFixed, Scale(302.0f));
          }
          GalleryCard("buttons", "Buttons", assets.heading_font(), DrawButtons);
          GalleryCard("availability", "Availability", assets.heading_font(),
                      [&state] { DrawAvailability(state); });
          GalleryCard(
              "workspace-switcher", "Workspace switcher", assets.heading_font(),
              [&chrome, &state] { DrawWorkspaceSwitcher(chrome, state); });
          GalleryCard("selection-scope", "Selection scope",
                      assets.heading_font(),
                      [&chrome, &state] { DrawSelectionScope(chrome, state); });
          GalleryCard("selection-tool", "Selection tool", assets.heading_font(),
                      [&chrome, &state] { DrawSelectionTool(chrome, state); });
          GalleryCard("inputs", "Inputs", assets.heading_font(),
                      [&state] { DrawInputs(state); });
          GalleryCard("slider", "Slider", assets.heading_font(),
                      [&state] { DrawSlider(state); });
          GalleryCard("compass", "Compass", assets.heading_font(),
                      [&state] { DrawCompass(state, false); });
          GalleryCard("compass-inherited", "Compass · inherited",
                      assets.heading_font(),
                      [&state] { DrawCompass(state, true); });
          GalleryCard("checkboxes", "Checkboxes", assets.heading_font(),
                      [&state] { DrawCheckboxes(state); });
          GalleryCard("visibility", "Visibility", assets.heading_font(),
                      [&assets, &state] { DrawVisibility(assets, state); });
          GalleryCard("enabled-locked", "Enabled & locked",
                      assets.heading_font(),
                      [&assets, &state] { DrawEnabledLocked(assets, state); });
          GalleryCard(
              "disclosure-rows", "Disclosure rows", assets.heading_font(),
              [&state, &assets] {
                DrawDisclosureRows(state, assets.heading_font());
              },
              false, false, 300.0f);
          GalleryCard(
              "navigation-primitives", "Navigation primitives",
              assets.heading_font(),
              [&state] { DrawNavigationPrimitives(state); }, false, false,
              300.0f);
          GalleryCard(
              "advanced-fields", "Advanced fields", assets.heading_font(),
              [&state] { DrawAdvancedFields(state); }, false, false, 300.0f);
          GalleryCard(
              "layout-feedback", "Layout & feedback", assets.heading_font(),
              [&state] { DrawLayoutAndFeedback(state); }, false, false, 300.0f);
          GalleryCard("spatial-overlay", "Spatial overlay",
                      assets.heading_font(), DrawSpatialOverlay, false, false,
                      300.0f);
          GalleryCard(
              "grain-gizmo", "Grain direction gizmo", assets.heading_font(),
              [&assets, &state] { DrawSpatialGizmo(assets, state); }, false,
              true, 448.0f, 604.0f);
          GalleryCard(
              "hierarchy-step", "STEP", assets.heading_font(),
              [&assets, &state] {
                DrawCanonicalHierarchy(assets, state.hierarchy_cards[0],
                                       kStepHierarchy, "STEP");
              },
              false, true, 448.0f, 604.0f);
          GalleryCard(
              "hierarchy-svg", "SVG", assets.heading_font(),
              [&assets, &state] {
                DrawCanonicalHierarchy(assets, state.hierarchy_cards[1],
                                       kSvgHierarchy, "SVG");
              },
              false, true, 448.0f, 604.0f);
          GalleryCard(
              "hierarchy-dxf", "DXF", assets.heading_font(),
              [&assets, &state] {
                DrawCanonicalHierarchy(assets, state.hierarchy_cards[2],
                                       kDxfHierarchy, "DXF");
              },
              false, true, 448.0f, 604.0f);
          GalleryCard(
              "hierarchy-canvas-issues", "Canvas Issues", assets.heading_font(),
              [&assets, &state] { DrawIssueHierarchy(assets, state); }, false,
              true, 448.0f, 604.0f);
          GalleryCard("status-types", "Status types", assets.heading_font(),
                      [&assets] { DrawStatusTypes(assets); });
          GalleryCard("operation", "Operation", assets.heading_font(),
                      [&assets] { DrawOperation(assets); });
          GalleryCard("empty-overflow", "Empty & overflow",
                      assets.heading_font(), DrawEmptyOverflow);
          GalleryCard("color-pickers", "Color pickers", assets.heading_font(),
                      [&state] { DrawColorPickers(state); });
          GalleryCard("progress", "Progress", assets.heading_font(),
                      DrawProgress);
          ImGui::EndTable();
        }
      }
      ImGui::EndChild();
    });
    panel(GalleryTab::Settings,
          [&assets, &state] { DrawSettingsGallery(assets, state); });
    panel(GalleryTab::Operations,
          [&assets, &state] { DrawOperationStateGallery(assets, state); });
    panel(GalleryTab::Status,
          [&assets, &state] { DrawStatusBarStateGallery(assets, state); });
  }
  ImGui::End();

  if (state.active_tab == GalleryTab::Settings &&
      (state.settings.window_open ||
       state.settings.discard_confirmation_open)) {
    DrawSettingsGalleryWindow(assets, state);
  }
}

} // namespace fancy_ui::gallery
