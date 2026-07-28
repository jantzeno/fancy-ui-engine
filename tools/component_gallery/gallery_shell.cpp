#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

namespace fancy_ui::gallery {

namespace {

void DrawApplicationBar(GalleryState &state) {
  ImGui::SetCursorPosY(Scale(4.0f));
  ImGui::TextUnformatted("File   Edit   View   Object   Tools   Help");
  ImGui::SameLine();
  ImGui::TextDisabled("Canvas");

  const float explorer_width = Scale(104.0f);
  const float inspector_width = Scale(104.0f);
  const float gap = Scale(8.0f);
  const float start = ImGui::GetWindowWidth() - explorer_width -
                      inspector_width - gap - Scale(12.0f);
  ImGui::SameLine(std::max(ImGui::GetCursorPosX(), start));
  const bool explorer_visible = state.shell.layout.explorer_visible;
  const ButtonResult explorer = Button({
      .id = "shell-toggle-explorer",
      .label = "Explorer",
      .tooltip = explorer_visible ? "Hide Explorer" : "Show Explorer",
      .variant = ButtonVariant::Tertiary,
      .selected = explorer_visible,
      .size = {.x = 104.0f, .y = 32.0f},
  });
  if (explorer.activated) {
    state.shell.layout.explorer_visible = !explorer_visible;
  }

  ImGui::SameLine();
  const bool inspector_visible = state.shell.layout.inspector_visible;
  const ButtonResult inspector = Button({
      .id = "shell-toggle-inspector",
      .label = "Inspector",
      .tooltip = inspector_visible ? "Hide Inspector" : "Show Inspector",
      .variant = ButtonVariant::Tertiary,
      .selected = inspector_visible,
      .size = {.x = 104.0f, .y = 32.0f},
  });
  if (inspector.activated) {
    state.shell.layout.inspector_visible = !inspector_visible;
  }
}

void DrawContextToolbar() {
  ImGui::SetCursorPos(ImVec2(Scale(12.0f), Scale(4.0f)));
  static_cast<void>(Button({
      .id = "scope-canvas",
      .label = "Canvas",
      .selected = true,
      .size = {.x = 72.0f, .y = 32.0f},
  }));
  ImGui::SameLine();
  static_cast<void>(Button({
      .id = "scope-object",
      .label = "Object",
      .size = {.x = 72.0f, .y = 32.0f},
  }));
  ImGui::SameLine(0.0f, Scale(16.0f));
  static_cast<void>(Button({
      .id = "tool-pointer",
      .label = "Pointer",
      .selected = true,
      .size = {.x = 80.0f, .y = 32.0f},
  }));
  ImGui::SameLine();
  static_cast<void>(Button({
      .id = "tool-rectangle",
      .label = "Rectangle",
      .size = {.x = 96.0f, .y = 32.0f},
  }));
  ImGui::SameLine();
  static_cast<void>(Button({
      .id = "tool-oval",
      .label = "Oval",
      .size = {.x = 64.0f, .y = 32.0f},
  }));

  const float right_width = Scale(208.0f);
  ImGui::SameLine(
      std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - right_width));
  static_cast<void>(Button({
      .id = "grid",
      .label = "Grid: 10 mm",
      .size = {.x = 112.0f, .y = 32.0f},
  }));
  ImGui::SameLine();
  static_cast<void>(Button({
      .id = "snap",
      .label = "Snap: On",
      .size = {.x = 88.0f, .y = 32.0f},
  }));
}

void DrawActivityRail(detail::UiAssetAtlas &assets,
                      const bool diagnostics_enabled) {
  struct Activity {
    std::string_view id;
    std::string_view label;
    std::string_view icon;
  };
  static constexpr std::array activities{
      Activity{"objects", "Objects", "objects"},
      Activity{"beds", "Beds", "bed"},
      Activity{"grain", "Grain", "grain"},
      Activity{"search", "Search", "search"},
      Activity{"compact", "Compact", "compact"},
  };
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
  for (const Activity &activity : activities) {
    static_cast<void>(NavigationItem({
        .id = activity.id,
        .label = activity.label,
        .tooltip = activity.label,
        .selected = activity.id == "objects",
        .draw_icon = assets.Painter(activity.icon),
    }));
  }
  if (diagnostics_enabled) {
    static_cast<void>(NavigationItem({
        .id = "diagnostics",
        .label = "Diagnostics",
        .tooltip = "Diagnostics",
        .draw_icon = assets.Painter("diagnostics"),
    }));
  }
  ImGui::PopStyleVar();
}

void DrawExplorer(detail::UiAssetAtlas &assets, GalleryState &state) {
  if (assets.bold_font() != nullptr) {
    ImGui::PushFont(assets.bold_font());
  }
  ImGui::TextUnformatted("Objects");
  if (assets.bold_font() != nullptr) {
    ImGui::PopFont();
  }
  ImGui::Separator();
  static_cast<void>(TextInput({
      .id = "shell-explorer-search",
      .label = "Search",
      .value = "",
  }));
  ImGui::Spacing();
  if (ImGui::BeginChild("##shell-hierarchy", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
    DrawHierarchySample(assets, state);
  }
  ImGui::EndChild();
}

void DrawWorkspace(detail::UiAssetAtlas &assets) {
  const float available_height = ImGui::GetContentRegionAvail().y;
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                       std::max(Scale(24.0f), available_height * 0.38f));
  EmptyState({
      .id = "blank-workspace",
      .title = "Blank view",
      .message = "No surface is bound in this gallery composition.",
      .icon = assets.Painter("objects"),
      .minimum_height = 96.0f,
  });
}

void DrawInspector(detail::UiAssetAtlas &assets, GalleryState &state) {
  ShellGalleryState &shell = state.shell;
  if (assets.bold_font() != nullptr) {
    ImGui::PushFont(assets.bold_font());
  }
  ImGui::TextUnformatted("Component examples");
  if (assets.bold_font() != nullptr) {
    ImGui::PopFont();
  }
  ImGui::Separator();

  if (ImGui::BeginChild("##shell-inspector-scroll", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
    const SectionResult values = BeginSection({
        .id = "inspector-values",
        .heading = "Values and fields",
    });
    if (values.visible) {
      static_cast<void>(ValueDisplay({
          .id = "source",
          .label = "Source",
          .value = "fixture-kit-07.svg",
      }));
      const TextInputResult name = TextInput({
          .id = "name",
          .label = "Name",
          .value = shell.object_name,
      });
      if (name.changed) {
        shell.object_name = name.value;
      }
      const NumericInputResult spacing = NumericInput({
          .id = "spacing",
          .label = "Spacing",
          .unit = "mm",
          .value = shell.spacing_mm,
          .minimum = 0.0,
          .format = "%.1f",
      });
      if (spacing.changed) {
        shell.spacing_mm = spacing.value;
      }
      static constexpr std::array materials{
          SelectOption{.id = "plywood", .label = "Plywood"},
          SelectOption{.id = "acrylic", .label = "Acrylic"},
          SelectOption{.id = "aluminum", .label = "Aluminum"},
      };
      const SelectResult material = Select({
          .id = "material",
          .label = "Material",
          .options = materials,
          .selected_index = shell.material_index,
      });
      if (material.changed) {
        shell.material_index = material.selected_index;
      }
      const DurationResult duration = Duration({
          .id = "duration",
          .label = "Time limit",
          .hours = shell.hours,
          .minutes = shell.minutes,
      });
      if (duration.changed) {
        shell.hours = duration.hours;
        shell.minutes = duration.minutes;
      }
    }
    EndSection(values);

    const SectionResult toggles = BeginSection({
        .id = "inspector-toggles",
        .heading = "Toggles and range",
    });
    if (toggles.visible) {
      const CheckboxResult enabled = Checkbox({
          .id = "enabled",
          .label = "Enabled",
          .state = shell.enabled,
      });
      if (enabled.changed) {
        shell.enabled = enabled.state;
      }
      const VisibilityToggleResult visible = VisibilityToggle({
          .id = "visible",
          .label = "Show overlay",
          .state = shell.visible,
          .visible_icon = assets.Painter("visibility"),
          .hidden_icon = assets.Painter("visibility-off"),
      });
      if (visible.changed) {
        shell.visible = visible.state;
      }
      const SliderResult explode = Slider({
          .id = "explode",
          .label = "Explode",
          .unit = "%",
          .value = shell.explode_percent,
          .minimum = 0.0f,
          .maximum = 100.0f,
          .format = "%.0f",
      });
      if (explode.changed) {
        shell.explode_percent = explode.value;
      }
    }
    EndSection(toggles);

    const SectionResult specialized = BeginSection({
        .id = "inspector-specialized",
        .heading = "Specialized",
    });
    if (specialized.visible) {
      const RotationCompassResult rotations = RotationCompass({
          .id = "rotations",
          .label = "Search rotations",
          .count = shell.rotation_count,
      });
      if (rotations.changed) {
        shell.rotation_count = rotations.count;
      }
      const std::span<const ColorRgba> color(&shell.display_color, 1);
      const ColorSwatchResult swatch = ColorSwatch(
          {
              .id = "display-color",
              .label = "Display color",
              .tooltip = "Open display color picker",
              .picker_title = "Display color",
              .value = shell.display_color,
              .colors = color,
          },
          shell.color_picker);
      if (swatch.changed) {
        shell.display_color = swatch.value;
      }
    }
    EndSection(specialized);

    const SectionResult feedback = BeginSection({
        .id = "inspector-feedback",
        .heading = "Feedback and action",
    });
    if (feedback.visible) {
      StatusCard({
          .id = "readiness",
          .title = "Ready",
          .message = "Inspector components are ready.",
          .status = SemanticStatus::Success,
          .icon = assets.Painter("success"),
      });
      if (Button({
                     .id = "inspect",
                     .label = "Inspect component",
                     .variant = ButtonVariant::Primary,
                     .size = {.x = -1.0f, .y = 32.0f},
                 })
              .activated) {
        shell.feedback = "Component inspection requested.";
      }
      ImGui::TextDisabled("%s", shell.feedback.c_str());
    }
    EndSection(feedback);
  }
  ImGui::EndChild();
}

} // namespace

void DrawApplicationShellGallery(detail::UiAssetAtlas &assets,
                                 GalleryState &state) {
  ImGui::TextDisabled(
      "Canonical nine-region shell; the application-bar controls preserve "
      "panel widths while the operation strip owns tray disclosure.");
  ImGui::Spacing();
  if (ImGui::BeginChild("##application-shell-gallery", ImVec2(0.0f, 0.0f),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoSavedSettings)) {
    shell::ApplicationShellState input = state.shell.layout;
    input.operation_tray_visible = state.shell.operation.expanded;
    const shell::ApplicationShellSpec spec{
        .application_bar =
            {
                .id = "gallery-application-bar",
                .draw = [&state]() { DrawApplicationBar(state); },
            },
        .context_toolbar =
            {
                .id = "gallery-context-toolbar",
                .draw = DrawContextToolbar,
            },
        .activity_rail =
            {
                .id = "gallery-activity-rail",
                .draw =
                    [&assets, &state]() {
                      DrawActivityRail(
                          assets,
                          state.settings.applied.general.diagnostics_enabled);
                    },
            },
        .explorer =
            {
                .id = "gallery-explorer",
                .draw = [&assets, &state]() { DrawExplorer(assets, state); },
            },
        .workspace =
            {
                .id = "gallery-workspace",
                .draw = [&assets]() { DrawWorkspace(assets); },
            },
        .inspector =
            {
                .id = "gallery-inspector",
                .draw = [&assets, &state]() { DrawInspector(assets, state); },
            },
        .operation_tray =
            {
                .id = "gallery-operation-tray",
                .draw = [&assets,
                         &state]() { DrawShellOperationTray(assets, state); },
            },
        .operation_strip =
            {
                .id = "gallery-operation-strip",
                .draw = [&assets,
                         &state]() { DrawShellOperationStrip(assets, state); },
            },
        .status_bar =
            {
                .id = "gallery-status-bar",
                .draw = [&assets,
                         &state]() { DrawShellStatusBar(assets, state); },
            },
    };
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
    const shell::ApplicationShellResult result =
        shell::Application(spec, input);
    ImGui::PopStyleVar();
    state.shell.layout.explorer_width = result.state.explorer_width;
    state.shell.layout.inspector_width = result.state.inspector_width;
    state.shell.layout.operation_tray_height =
        state.shell.operation.tray_height;
    state.shell.layout.operation_tray_visible = state.shell.operation.expanded;
  }
  ImGui::EndChild();
}

} // namespace fancy_ui::gallery
