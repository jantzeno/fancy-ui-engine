#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <array>
#include <functional>
#include <span>
#include <string>

namespace fancy_ui::gallery {

namespace {

struct ExampleState {
  double spacing = 8.0;
  std::size_t rotation_option = 0;
  int hours = 0;
  int minutes = 5;
  float explode = 38.0f;
  int rotations = 8;
  ToggleState checkbox = ToggleState::On;
  ToggleState visible = ToggleState::On;
  ToggleState enabled = ToggleState::On;
  ToggleState locked = ToggleState::On;
  bool assembly_expanded = true;
};

ExampleState examples;

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
                 const std::function<void()> &draw) {
  ImGui::TableNextColumn();
  ImGui::PushID(id);
  ImGui::PushStyleColor(
      ImGuiCol_ChildBg,
      ImVec4(CurrentPalette().surface.red, CurrentPalette().surface.green,
             CurrentPalette().surface.blue, CurrentPalette().surface.alpha));
  if (ImGui::BeginChild("##card", ImVec2(Scale(300.0f), Scale(236.0f)),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar)) {
    Heading(title, heading_font);
    draw();
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::PopID();
}

void PreviewButton(const char *id, const char *label,
                   const detail::InteractionPreview preview) {
  const detail::ScopedInteractionPreview state(preview);
  static_cast<void>(Button({
      .id = id,
      .label = label,
      .size = {.x = 64.0f, .y = 32.0f},
  }));
}

void DrawButtons() {
  PreviewButton("default", "Default", detail::InteractionPreview::Rest);
  ImGui::SameLine();
  PreviewButton("hovered", "Hovered", detail::InteractionPreview::Hovered);
  ImGui::SameLine();
  PreviewButton("pressed", "Pressed", detail::InteractionPreview::Pressed);
  ImGui::SameLine();
  PreviewButton("focused", "Focused", detail::InteractionPreview::Focused);
}

void DrawAvailability() {
  static_cast<void>(Button({
      .id = "selected",
      .label = "Selected",
      .selected = true,
      .size = {.x = 82.0f, .y = 32.0f},
  }));
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
  static_cast<void>(Button({
      .id = "invalid",
      .label = "Invalid",
      .validation =
          {
              .invalid = true,
          },
      .size = {.x = 76.0f, .y = 32.0f},
  }));
  ImGui::Spacing();
  ImGui::TextDisabled("Disabled - select an eligible object");
}

void DrawInputs() {
  const NumericInputResult spacing = NumericInput({
      .id = "spacing",
      .label = "Spacing",
      .unit = "mm",
      .value = examples.spacing,
      .minimum = 0.0,
      .format = "%.1f",
  });
  if (spacing.changed) {
    examples.spacing = spacing.value;
  }
  static constexpr std::array options{
      SelectOption{.id = "quarters", .label = "0°, 90°"},
      SelectOption{.id = "every-quarter", .label = "Every 90°"},
  };
  const SelectResult rotation = Select({
      .id = "rotation",
      .label = "Rotation",
      .options = options,
      .selected_index = examples.rotation_option,
  });
  if (rotation.changed) {
    examples.rotation_option = rotation.selected_index;
  }
  const DurationResult duration = Duration({
      .id = "duration",
      .label = "Duration",
      .hours = examples.hours,
      .minutes = examples.minutes,
  });
  if (duration.changed) {
    examples.hours = duration.hours;
    examples.minutes = duration.minutes;
  }
}

void DrawSlider() {
  const SliderResult result = Slider({
      .id = "explode",
      .label = "Explode",
      .unit = "%",
      .value = examples.explode,
      .minimum = 0.0f,
      .maximum = 100.0f,
      .format = "%.0f",
  });
  if (result.changed) {
    examples.explode = result.value;
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

void DrawCompass(const bool inherited) {
  const RotationCompassResult result = RotationCompass({
      .id = inherited ? "inherited" : "local",
      .label = inherited ? "Allowed rotations" : "Rotations",
      .count = examples.rotations,
      .inherited = inherited,
      .availability =
          inherited ? Availability{.enabled = false,
                                   .reason = "Inherited from bed settings"}
                    : Availability{},
  });
  if (result.changed) {
    examples.rotations = result.count;
  }
}

void DrawCheckboxes() {
  const CheckboxResult state = Checkbox({
      .id = "on",
      .label = "Enabled",
      .state = examples.checkbox,
  });
  if (state.changed) {
    examples.checkbox = state.state;
  }
  static_cast<void>(Checkbox({
      .id = "off",
      .label = "Disabled",
      .state = ToggleState::Off,
  }));
  static_cast<void>(Checkbox({
      .id = "mixed",
      .label = "Mixed",
      .state = ToggleState::Mixed,
  }));
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

void DrawVisibility(detail::UiAssetAtlas &assets) {
  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");
  const VisibilityToggleResult result = VisibilityToggle({
      .id = "visible",
      .label = "Overlay",
      .state = examples.visible,
      .visible_icon = visible,
      .hidden_icon = hidden,
  });
  if (result.changed) {
    examples.visible = result.state;
  }
  static_cast<void>(VisibilityToggle({
      .id = "hidden",
      .label = "Guides",
      .state = ToggleState::Off,
      .visible_icon = visible,
      .hidden_icon = hidden,
  }));
  static_cast<void>(VisibilityToggle({
      .id = "mixed",
      .label = "Selection",
      .state = ToggleState::Mixed,
      .visible_icon = visible,
      .hidden_icon = hidden,
  }));
}

void DrawEnabledLocked(detail::UiAssetAtlas &assets) {
  const IconPainter enabled = assets.Painter("check");
  const IconPainter locked = assets.Painter("orbit-locked");
  const IconPainter unlocked = assets.Painter("orbit-unlocked");
  const CheckboxResult enabled_result = Checkbox({
      .id = "enabled",
      .label = "Direction enabled",
      .state = examples.enabled,
      .on_icon = enabled,
  });
  if (enabled_result.changed) {
    examples.enabled = enabled_result.state;
  }
  const CheckboxResult locked_result = Checkbox({
      .id = "locked",
      .label = "Direction locked",
      .state = examples.locked,
      .on_icon = locked,
      .off_icon = unlocked,
  });
  if (locked_result.changed) {
    examples.locked = locked_result.state;
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

void DrawTreeRows(detail::UiAssetAtlas &assets) {
  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");
  const IconPainter more = assets.Painter("more");
  const HierarchyRowResult assembly = HierarchyRow({
      .id = "assembly",
      .label = "Front housing",
      .secondary_label = "Assembly",
      .expandable = true,
      .expanded = examples.assembly_expanded,
      .selected = true,
      .action_icon = more,
      .visibility = ToggleState::On,
      .visible_icon = visible,
      .hidden_icon = hidden,
  });
  if (assembly.expansion_changed) {
    examples.assembly_expanded = assembly.expanded;
  }
  static_cast<void>(HierarchyRow({
      .id = "part",
      .label = "Face plate",
      .secondary_label = "Part 4",
      .depth = 1,
      .color = ColorRgba{.red = 0.27f, .green = 0.58f, .blue = 0.97f},
      .visibility = ToggleState::Off,
      .visible_icon = visible,
      .hidden_icon = hidden,
  }));
  {
    const detail::ScopedInteractionPreview focus(
        detail::InteractionPreview::Focused);
    static_cast<void>(HierarchyRow({
        .id = "path",
        .label = "Outer contour",
        .secondary_label = "Path 184",
        .depth = 2,
        .action_icon = more,
        .visibility = ToggleState::On,
        .visible_icon = visible,
        .hidden_icon = hidden,
    }));
  }
}

void DrawIssueHierarchy(detail::UiAssetAtlas &assets) {
  const IconPainter more = assets.Painter("more");
  const IconPainter visible = assets.Painter("visibility");
  const IconPainter hidden = assets.Painter("visibility-off");
  static_cast<void>(HierarchyRow({
      .id = "warning",
      .label = "Open contours",
      .secondary_label = "4 issues",
      .expandable = true,
      .expanded = true,
      .status = SemanticStatus::Warning,
      .action_icon = more,
  }));
  static_cast<void>(HierarchyRow({
      .id = "failure",
      .label = "Orphan hole",
      .secondary_label = "Path 184 - invalid",
      .depth = 1,
      .status = SemanticStatus::Failure,
      .color = ColorRgba{.red = 0.97f, .green = 0.32f, .blue = 0.29f},
      .action_icon = more,
      .visibility = ToggleState::Off,
      .visible_icon = visible,
      .hidden_icon = hidden,
  }));
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

void DrawOperation(detail::UiAssetAtlas &assets) {
  if (ImGui::BeginTable("##operation-status", 2,
                        ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    StatusCard({
        .id = "preview",
        .message = "8 pieces ready",
        .status = SemanticStatus::Preview,
        .icon = assets.Painter("visibility"),
    });
    ImGui::TableNextColumn();
    StatusCard({
        .id = "busy",
        .message = "Iteration 24",
        .status = SemanticStatus::Busy,
        .icon = assets.Painter("busy"),
    });
    ImGui::EndTable();
  }
  ImGui::Spacing();
  ProgressBar({
      .id = "search-progress",
      .label = "Search progress: 62%",
      .value = 0.62f,
  });
  ImGui::Spacing();
  static_cast<void>(Button({
      .id = "pause",
      .label = "Pause",
      .size = {.x = 72.0f, .y = 28.0f},
  }));
  ImGui::SameLine();
  static_cast<void>(Button({
      .id = "stop",
      .label = "Stop",
      .variant = ButtonVariant::Destructive,
      .size = {.x = 72.0f, .y = 28.0f},
  }));
}

void DrawMixedValues(detail::UiAssetAtlas &assets) {
  static_cast<void>(ValueDisplay({
      .id = "margin",
      .label = "Margin",
      .mixed = true,
  }));
  ImGui::Spacing();
  static constexpr std::array colors{
      ColorRgba{.red = 0.27f, .green = 0.58f, .blue = 0.97f},
      ColorRgba{.red = 0.64f, .green = 0.44f, .blue = 0.97f},
  };
  static_cast<void>(ColorSwatch({
      .id = "color",
      .label = "Bed color",
      .colors = colors,
  }));
  static_cast<void>(Checkbox({
      .id = "checkbox",
      .label = "Margins - Mixed",
      .state = ToggleState::Mixed,
  }));
  static_cast<void>(VisibilityToggle({
      .id = "visibility",
      .label = "Visibility",
      .state = ToggleState::Mixed,
      .visible_icon = assets.Painter("visibility"),
      .hidden_icon = assets.Painter("visibility-off"),
  }));
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
  ImGui::TextUnformatted("Component states");
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

  const float table_width = Scale(4.0f * 300.0f + 3.0f * 8.0f);
  if (ImGui::BeginChild("##gallery-scroll", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    if (ImGui::BeginTable("##component-grid", 4, ImGuiTableFlags_SizingFixedFit,
                          ImVec2(table_width, 0.0f))) {
      for (int column = 0; column < 4; ++column) {
        ImGui::TableSetupColumn("component", ImGuiTableColumnFlags_WidthFixed,
                                Scale(300.0f));
      }
      GalleryCard("buttons", "Buttons", assets.bold_font(), DrawButtons);
      GalleryCard("availability", "Availability", assets.bold_font(),
                  DrawAvailability);
      GalleryCard("inputs", "Inputs", assets.bold_font(), DrawInputs);
      GalleryCard("slider", "Slider", assets.bold_font(), DrawSlider);
      GalleryCard("compass", "Compass", assets.bold_font(),
                  [] { DrawCompass(false); });
      GalleryCard("compass-inherited", "Compass inherited", assets.bold_font(),
                  [] { DrawCompass(true); });
      GalleryCard("checkboxes", "Checkboxes", assets.bold_font(),
                  DrawCheckboxes);
      GalleryCard("visibility", "Visibility", assets.bold_font(),
                  [&assets] { DrawVisibility(assets); });
      GalleryCard("enabled-locked", "Enabled & locked", assets.bold_font(),
                  [&assets] { DrawEnabledLocked(assets); });
      GalleryCard("tree-rows", "Tree rows", assets.bold_font(),
                  [&assets] { DrawTreeRows(assets); });
      GalleryCard("issue-hierarchy", "Issue hierarchy", assets.bold_font(),
                  [&assets] { DrawIssueHierarchy(assets); });
      GalleryCard("status-types", "Status types", assets.bold_font(),
                  [&assets] { DrawStatusTypes(assets); });
      GalleryCard("operation", "Operation", assets.bold_font(),
                  [&assets] { DrawOperation(assets); });
      GalleryCard("mixed-values", "Mixed values", assets.bold_font(),
                  [&assets] { DrawMixedValues(assets); });
      GalleryCard("empty-overflow", "Empty & overflow", assets.bold_font(),
                  DrawEmptyOverflow);
      ImGui::EndTable();
    }
  }
  ImGui::EndChild();
  ImGui::End();
}

} // namespace fancy_ui::gallery
