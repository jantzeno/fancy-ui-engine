#include "internal/application_chrome.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace fancy_ui::detail {

namespace {

using namespace steppenface;

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

bool Available(const steppenface::Availability &availability) {
  return availability.visible && availability.enabled && !availability.busy;
}

struct WorkspaceSegmentInteraction {
  const char *label = "";
  bool hovered = false;
  bool pressed = false;
  bool keyboard_focused = false;
};

struct ToolbarSegmentInteraction {
  const ToolbarChoiceView *choice = nullptr;
  ImVec2 minimum;
  ImVec2 maximum;
  bool hovered = false;
  bool pressed = false;
  bool keyboard_focused = false;
  bool disabled = false;
};

} // namespace

class ApplicationChrome::Impl {
public:
  explicit Impl(UiAssetAtlas &assets) : assets_(assets) {}

  UiAssetAtlas &assets_;

  void Invoke(const CommandView &command,
              const ApplicationChromeCallbacks &callbacks) const {
    if (Available(command.availability) && callbacks.invoke_command) {
      callbacks.invoke_command(command);
    }
  }

  void Commit(const ControlActionView &action,
              const ApplicationChromeCallbacks &callbacks) const {
    if (Available(action.availability) && callbacks.commit_action) {
      callbacks.commit_action(action);
    }
  }

  void DrawUnavailableMenuCommand(const CommandView &command) const {
    const char *shortcut =
        command.shortcut.empty() ? nullptr : command.shortcut.c_str();
    const ImVec4 transparent(0.0f, 0.0f, 0.0f, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::PushStyleColor(ImGuiCol_Header, transparent);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, transparent);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, transparent);
    ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
    static_cast<void>(
        ImGui::MenuItem(command.label.c_str(), shortcut, false, true));
    ImGui::PopItemFlag();
    ImGui::PopStyleColor(4);

    const bool keyboard_focused =
        ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
    if ((ImGui::IsItemHovered() || keyboard_focused) &&
        !command.availability.disabled_reason.empty()) {
      ShowTooltip(command.availability.disabled_reason);
    }
  }

  void DrawMenuItems(const ApplicationBarView &bar,
                     const std::vector<MenuItemView> &items,
                     const ApplicationChromeCallbacks &callbacks,
                     const LayoutMetrics &metrics) {
    for (const MenuItemView &item : items) {
      switch (item.kind) {
      case MenuItemKind::Separator:
        ImGui::Separator();
        break;
      case MenuItemKind::Submenu:
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(metrics.menu.popup_width, 0.0f),
            ImVec2(metrics.menu.popup_width,
                   std::numeric_limits<float>::max()));
        if (ImGui::BeginMenu(item.label.c_str())) {
          DrawMenuItems(bar, item.children, callbacks, metrics);
          ImGui::EndMenu();
        }
        break;
      case MenuItemKind::Workspace:
        if (item.workspace.has_value() &&
            ImGui::MenuItem(item.label.c_str(), nullptr,
                            *item.workspace == bar.active_workspace, true) &&
            callbacks.activate_workspace) {
          callbacks.activate_workspace(*item.workspace);
        }
        break;
      case MenuItemKind::Command:
        if (!item.command.has_value() || !item.command->availability.visible) {
          break;
        }
        {
          const CommandView &command = *item.command;
          const bool enabled = Available(command.availability);
          const char *shortcut =
              command.shortcut.empty() ? nullptr : command.shortcut.c_str();
          if (!enabled) {
            DrawUnavailableMenuCommand(command);
          } else if (ImGui::MenuItem(command.label.c_str(), shortcut, false,
                                     true)) {
            Invoke(command, callbacks);
          }
        }
        break;
      }
    }
  }

  bool BeginApplicationMenu(const char *label,
                            const LayoutMetrics &metrics) const {
    const SemanticPalette &palette = CurrentPalette();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(1);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                          ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,
                          ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    const bool open = ImGui::BeginMenu(label);
    ImGui::PopStyleColor(3);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    draw_list->ChannelsSetCurrent(0);
    if (open || hovered) {
      draw_list->AddRectFilled(
          minimum, maximum,
          ImGui::GetColorU32(
              ToImVec4(open ? palette.control_pressed : palette.control_hover)),
          metrics.menu.trigger_rounding);
    }
    draw_list->ChannelsMerge();
    return open;
  }

  WorkspaceSegmentInteraction
  CaptureWorkspaceSegment(const WorkspaceKind workspace, const char *label,
                          const ApplicationChromeCallbacks &callbacks,
                          const LayoutMetrics &metrics) const {
    ImGui::PushID(workspace == WorkspaceKind::Model3d ? "workspace.3d"
                                                      : "workspace.canvas");
    if (ImGui::InvisibleButton(
            "##segment", ImVec2(Scale(72.0f), metrics.geometry.control_height),
            ImGuiButtonFlags_EnableNav) &&
        callbacks.activate_workspace) {
      callbacks.activate_workspace(workspace);
    }
    const WorkspaceSegmentInteraction interaction{
        .label = label,
        .hovered = ImGui::IsItemHovered(),
        .pressed = ImGui::IsItemActive(),
        .keyboard_focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible,
    };
    ImGui::PopID();
    return interaction;
  }

  void DrawWorkspaceSwitcher(const ApplicationBarView &bar,
                             const ApplicationChromeCallbacks &callbacks,
                             const LayoutMetrics &metrics) const {
    const float segment_width = Scale(72.0f);
    const SemanticPalette &palette = CurrentPalette();
    const ImVec2 minimum = ImGui::GetCursorScreenPos();
    const WorkspaceSegmentInteraction model = CaptureWorkspaceSegment(
        WorkspaceKind::Model3d, "3D", callbacks, metrics);
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetCursorScreenPos(ImVec2(minimum.x + segment_width, minimum.y));
    const WorkspaceSegmentInteraction canvas = CaptureWorkspaceSegment(
        WorkspaceKind::Canvas, "Canvas", callbacks, metrics);
    const ImVec2 maximum(minimum.x + segment_width * 2.0f,
                         minimum.y + metrics.geometry.control_height);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const std::array interactions{model, canvas};
    for (std::size_t index = 0; index < interactions.size(); ++index) {
      const WorkspaceSegmentInteraction &interaction = interactions[index];
      const WorkspaceKind workspace =
          index == 0 ? WorkspaceKind::Model3d : WorkspaceKind::Canvas;
      const bool selected = workspace == bar.active_workspace;
      const ColorRgba fill = selected              ? palette.selection
                             : interaction.pressed ? palette.control_pressed
                             : interaction.hovered ? palette.control_hover
                                                   : palette.surface;
      const ImVec2 segment_minimum(
          minimum.x + segment_width * static_cast<float>(index), minimum.y);
      const ImVec2 segment_maximum(segment_minimum.x + segment_width,
                                   maximum.y);
      const ImDrawFlags corners = index == 0 ? ImDrawFlags_RoundCornersLeft
                                             : ImDrawFlags_RoundCornersRight;
      draw_list->AddRectFilled(segment_minimum, segment_maximum,
                               ImGui::GetColorU32(ToImVec4(fill)),
                               metrics.geometry.surface_radius, corners);
      const ImVec2 label_size = ImGui::CalcTextSize(interaction.label);
      draw_list->AddText(
          ImVec2(std::floor(
                     (segment_minimum.x + segment_maximum.x - label_size.x) *
                     0.5f),
                 std::floor(
                     (segment_minimum.y + segment_maximum.y - label_size.y) *
                     0.5f)),
          ImGui::GetColorU32(ToImVec4(selected ? palette.text_primary
                                               : palette.text_secondary)),
          interaction.label);
      if (selected) {
        draw_list->AddRectFilled(
            ImVec2(segment_minimum.x + metrics.geometry.border,
                   maximum.y - Scale(3.0f)),
            ImVec2(segment_maximum.x - metrics.geometry.border,
                   maximum.y - metrics.geometry.border),
            ImGui::GetColorU32(ToImVec4(palette.focus)));
      }
      if (interaction.keyboard_focused) {
        draw_list->AddRect(ImVec2(segment_minimum.x + Scale(3.0f),
                                  segment_minimum.y + Scale(3.0f)),
                           ImVec2(segment_maximum.x - Scale(3.0f),
                                  segment_maximum.y - Scale(3.0f)),
                           ImGui::GetColorU32(ToImVec4(palette.focus)),
                           metrics.geometry.focus_ring, corners,
                           metrics.geometry.focus_ring);
      }
    }
    draw_list->AddRect(minimum, maximum,
                       ImGui::GetColorU32(ToImVec4(palette.border)),
                       metrics.geometry.surface_radius,
                       ImDrawFlags_RoundCornersAll, metrics.geometry.border);
    const float separator_x = minimum.x + segment_width;
    draw_list->AddLine(
        ImVec2(separator_x, minimum.y), ImVec2(separator_x, maximum.y),
        ImGui::GetColorU32(ToImVec4(palette.border)), metrics.geometry.border);
  }

  void DrawLayoutIcon(const std::string_view semantic_id, const ImVec2 minimum,
                      const ImVec2 maximum, const ColorRgba color) const {
    const float icon_size = CurrentLayoutMetrics().geometry.icon;
    const ImVec2 center((minimum.x + maximum.x) * 0.5f,
                        (minimum.y + maximum.y) * 0.5f);
    static_cast<void>(
        assets_.DrawIcon(semantic_id, UiIconSize::Small16,
                         {.minimum = {.x = center.x - icon_size * 0.5f,
                                      .y = center.y - icon_size * 0.5f},
                          .maximum = {.x = center.x + icon_size * 0.5f,
                                      .y = center.y + icon_size * 0.5f}},
                         color));
  }

  void DrawLayoutControls(const ChromeLayoutState &layout,
                          const ApplicationChromeCallbacks &callbacks,
                          const LayoutMetrics &metrics) const {
    struct Control {
      LayoutRegion region;
      const char *name;
      bool visible;
      bool enabled;
      const char *icon_open;
      const char *icon_closed;
    };
    const std::array controls{
        Control{LayoutRegion::Explorer, "Explorer", layout.explorer_visible,
                true, "layout-explorer-open", "layout-explorer-closed"},
        Control{LayoutRegion::OperationTray, "operation details",
                layout.operation_tray_visible, layout.operation_available,
                "layout-operation-open", "layout-operation-closed"},
        Control{LayoutRegion::Inspector, "Inspector", layout.inspector_visible,
                true, "layout-inspector-open", "layout-inspector-closed"},
    };

    const SemanticPalette &palette = CurrentPalette();
    const ImVec2 group_minimum = ImGui::GetCursorScreenPos();
    ImVec2 group_maximum = group_minimum;
    for (std::size_t index = 0; index < controls.size(); ++index) {
      if (index > 0) {
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetCursorScreenPos(
            ImVec2(group_minimum.x + metrics.geometry.control_height *
                                         static_cast<float>(index),
                   group_minimum.y));
      }
      const Control &control = controls[index];
      ImGui::PushID(static_cast<int>(control.region));
      ImGui::BeginDisabled(!control.enabled);
      const bool activated =
          ImGui::InvisibleButton("##layout-control",
                                 ImVec2(metrics.geometry.control_height,
                                        metrics.geometry.control_height),
                                 ImGuiButtonFlags_EnableNav);
      const bool hovered = ImGui::IsItemHovered();
      const bool pressed = ImGui::IsItemActive();
      const bool focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
      const ImVec2 minimum = ImGui::GetItemRectMin();
      const ImVec2 maximum = ImGui::GetItemRectMax();
      ImGui::EndDisabled();

      const ColorRgba fill = !control.enabled  ? palette.surface_muted
                             : control.visible ? palette.selection
                             : pressed         ? palette.control_pressed
                             : hovered         ? palette.control_hover
                                               : palette.application_surface;
      const ColorRgba foreground = !control.enabled  ? palette.text_disabled
                                   : control.visible ? palette.focus
                                                     : palette.text_secondary;
      const ImDrawFlags corners = index == 0 ? ImDrawFlags_RoundCornersLeft
                                  : index + 1 == controls.size()
                                      ? ImDrawFlags_RoundCornersRight
                                      : ImDrawFlags_RoundCornersNone;
      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      draw_list->AddRectFilled(minimum, maximum,
                               ImGui::GetColorU32(ToImVec4(fill)),
                               metrics.geometry.surface_radius, corners);
      if (index > 0) {
        draw_list->AddLine(minimum, ImVec2(minimum.x, maximum.y),
                           ImGui::GetColorU32(ToImVec4(palette.border)),
                           metrics.geometry.border);
      }
      DrawLayoutIcon(control.visible ? control.icon_open : control.icon_closed,
                     minimum, maximum, foreground);
      if (focused) {
        draw_list->AddRect(
            ImVec2(minimum.x + Scale(3.0f), minimum.y + Scale(3.0f)),
            ImVec2(maximum.x - Scale(3.0f), maximum.y - Scale(3.0f)),
            ImGui::GetColorU32(ToImVec4(palette.focus)),
            metrics.geometry.control_radius, ImDrawFlags_RoundCornersAll,
            metrics.geometry.focus_ring);
      }
      if (hovered || focused ||
          (!control.enabled &&
           ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))) {
        if (!control.enabled) {
          ShowTooltip("No operation details available");
        } else {
          ShowTooltip(std::string(control.visible ? "Hide " : "Show ") +
                      control.name);
        }
      }
      if (activated && control.enabled && callbacks.toggle_layout) {
        callbacks.toggle_layout(control.region);
      }
      group_maximum = maximum;
      ImGui::PopID();
    }
    ImGui::GetWindowDrawList()->AddRect(
        group_minimum, group_maximum,
        ImGui::GetColorU32(ToImVec4(palette.border)),
        metrics.geometry.surface_radius, ImDrawFlags_RoundCornersAll,
        metrics.geometry.border);
  }

  void DrawApplicationBar(const ApplicationBarView &bar,
                          const ChromeLayoutState &layout,
                          const ApplicationChromeCallbacks &callbacks,
                          const ApplicationBarHost host) {
    const LayoutMetrics metrics = CurrentLayoutMetrics();
    ImFont *menu_font = assets_.regular_font();
    ImGui::PushFont(menu_font, metrics.menu.font_size);
    const float vertical_padding = std::max(
        0.0f,
        (metrics.shell.application_bar_height - ImGui::GetFontSize()) * 0.5f);
    bool host_open = true;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(metrics.spacing.space04, vertical_padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(metrics.spacing.space04, 0.0f));
    if (host == ApplicationBarHost::MainViewport) {
      host_open = ImGui::BeginMainMenuBar();
    } else {
      host_open = ImGui::BeginMenuBar();
    }
    ImGui::PopStyleVar(2);
    if (!host_open) {
      ImGui::PopFont();
      return;
    }

    const SemanticPalette &palette = CurrentPalette();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(metrics.spacing.space03, Scale(7.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(metrics.menu.popup_padding_horizontal,
                               metrics.menu.popup_padding_vertical));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(metrics.spacing.space03,
                               std::max(0.0f, metrics.geometry.control_height -
                                                  ImGui::GetFontSize())));
    ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(palette.border_strong));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,
                          ToImVec4(palette.application_surface));
    for (const ApplicationMenuView &menu : bar.menus) {
      ImGui::SetNextWindowSizeConstraints(
          ImVec2(metrics.menu.popup_width, 0.0f),
          ImVec2(metrics.menu.popup_width, std::numeric_limits<float>::max()));
      if (BeginApplicationMenu(menu.label.c_str(), metrics)) {
        ImGui::PushStyleVar(
            ImGuiStyleVar_ItemSpacing,
            ImVec2(metrics.spacing.space03, metrics.spacing.space03));
        DrawMenuItems(bar, menu.items, callbacks, metrics);
        ImGui::PopStyleVar();
        ImGui::EndMenu();
      }
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    ImGui::PopFont();

    const float switcher_x = ImGui::GetCursorPosX();
    ImGui::SetCursorPos(ImVec2(switcher_x, metrics.spacing.space02));
    DrawWorkspaceSwitcher(bar, callbacks, metrics);

    if (bar.document_dirty && !bar.dirty_label.empty()) {
      const ImVec2 switcher_minimum = ImGui::GetItemRectMin();
      const ImVec2 switcher_maximum = ImGui::GetItemRectMax();
      const float center_y = (switcher_minimum.y + switcher_maximum.y) * 0.5f;
      const float marker_x = switcher_maximum.x + metrics.spacing.space05;
      const ImVec2 text_size = ImGui::CalcTextSize(bar.dirty_label.c_str());
      ImGui::GetWindowDrawList()->AddCircleFilled(
          ImVec2(marker_x, center_y), metrics.spacing.space02,
          ImGui::GetColorU32(ToImVec4(palette.warning)));
      ImGui::GetWindowDrawList()->AddText(
          ImVec2(marker_x + metrics.spacing.space04,
                 center_y - text_size.y * 0.5f),
          ImGui::GetColorU32(ToImVec4(palette.warning)),
          bar.dirty_label.c_str());
    }

    const float controls_width = metrics.geometry.control_height * 3.0f;
    ImGui::SetCursorPos(
        ImVec2(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() -
                                                    controls_width -
                                                    metrics.spacing.space04),
               metrics.spacing.space02));
    DrawLayoutControls(layout, callbacks, metrics);

    const ImVec2 window_position = ImGui::GetWindowPos();
    const ImVec2 window_size = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(window_position.x, window_position.y +
                                      metrics.shell.application_bar_height -
                                      metrics.geometry.border),
        ImVec2(window_position.x + window_size.x,
               window_position.y + metrics.shell.application_bar_height -
                   metrics.geometry.border),
        ImGui::GetColorU32(ToImVec4(palette.border)), metrics.geometry.border);

    if (host == ApplicationBarHost::MainViewport) {
      ImGui::EndMainMenuBar();
    } else {
      ImGui::EndMenuBar();
    }
  }

  void DrawToolbarIcon(const std::string &icon, const ImVec2 minimum,
                       const ImVec2 maximum, const ColorRgba color,
                       const LayoutMetrics &metrics) const {
    const ImVec2 center((minimum.x + maximum.x) * 0.5f,
                        (minimum.y + maximum.y) * 0.5f);
    const float half = metrics.geometry.icon * 0.5f;
    static_cast<void>(assets_.DrawIcon(
        icon, UiIconSize::Small16,
        {.minimum = {.x = center.x - half, .y = center.y - half},
         .maximum = {.x = center.x + half, .y = center.y + half}},
        color));
  }

  bool ToolbarButton(const std::string &id, const std::string &label,
                     const std::string &icon, const std::string &tooltip,
                     const steppenface::Availability &availability,
                     const bool selected, const bool icon_only,
                     const CommandVariant variant,
                     const LayoutMetrics &metrics) const {
    if (!availability.visible) {
      return false;
    }
    const SemanticPalette &palette = CurrentPalette();
    const bool disabled = !availability.enabled || availability.busy;
    const ImVec2 size =
        icon_only ? ImVec2(metrics.geometry.control_height,
                           metrics.geometry.control_height)
                  : ImVec2(std::max(Scale(56.0f),
                                    ImGui::CalcTextSize(label.c_str()).x +
                                        metrics.spacing.space06),
                           metrics.geometry.control_height);

    ImGui::PushID(id.c_str());
    ImGui::BeginDisabled(disabled);
    const bool activated =
        ImGui::InvisibleButton("##button", size, ImGuiButtonFlags_EnableNav);
    const bool hovered = ImGui::IsItemHovered();
    const bool pressed = ImGui::IsItemActive();
    const bool focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    ImGui::EndDisabled();

    ColorRgba fill = palette.control;
    ColorRgba border = icon_only ? palette.border : palette.border_strong;
    ColorRgba foreground = palette.text_primary;
    if (disabled) {
      fill = palette.control_disabled_fill;
      border = palette.control_disabled_border;
      foreground = palette.text_disabled;
    } else if (selected) {
      fill = palette.selection;
      border = palette.focus;
      foreground = palette.focus;
    } else if (variant == CommandVariant::Primary) {
      fill = pressed   ? palette.action_primary_pressed
             : hovered ? palette.action_primary_hover
                       : palette.action_primary;
      border = fill;
      foreground = palette.on_emphasis;
    } else {
      if (pressed) {
        fill = palette.control_pressed;
      } else if (hovered) {
        fill = palette.control_hover;
      }
      if (variant == CommandVariant::Tertiary && !hovered && !pressed) {
        fill.alpha = 0.0f;
        border.alpha = 0.0f;
      } else if (variant == CommandVariant::Destructive) {
        foreground = palette.failure;
      }
    }

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(minimum, maximum,
                             ImGui::GetColorU32(ToImVec4(fill)),
                             metrics.geometry.surface_radius);
    draw_list->AddRect(minimum, maximum, ImGui::GetColorU32(ToImVec4(border)),
                       metrics.geometry.surface_radius,
                       ImDrawFlags_RoundCornersAll, metrics.geometry.border);
    if (icon_only) {
      DrawToolbarIcon(icon, minimum, maximum, foreground, metrics);
    } else {
      const ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
      draw_list->AddText(
          ImVec2(std::floor((minimum.x + maximum.x - label_size.x) * 0.5f),
                 std::floor((minimum.y + maximum.y - label_size.y) * 0.5f -
                            Scale(1.0f))),
          ImGui::GetColorU32(ToImVec4(foreground)), label.c_str());
    }
    if (focused) {
      draw_list->AddRect(
          ImVec2(minimum.x + Scale(3.0f), minimum.y + Scale(3.0f)),
          ImVec2(maximum.x - Scale(3.0f), maximum.y - Scale(3.0f)),
          ImGui::GetColorU32(ToImVec4(palette.focus)),
          metrics.geometry.surface_radius, ImDrawFlags_RoundCornersAll,
          metrics.geometry.focus_ring);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      const std::string &message =
          availability.enabled ? tooltip : availability.disabled_reason;
      if (!message.empty()) {
        ShowTooltip(message);
      }
    }
    ImGui::PopID();
    return activated && !disabled;
  }

  void DrawToolbarSegmented(const ToolbarSegmentedView &segmented,
                            const ApplicationChromeCallbacks &callbacks,
                            const LayoutMetrics &metrics) const {
    std::vector<const ToolbarChoiceView *> choices;
    for (const ToolbarChoiceView &choice : segmented.choices) {
      if (choice.action.availability.visible) {
        choices.push_back(&choice);
      }
    }
    if (choices.empty()) {
      return;
    }

    ImGui::PushID(segmented.id.value.c_str());
    std::vector<ToolbarSegmentInteraction> interactions;
    for (std::size_t index = 0; index < choices.size(); ++index) {
      if (index > 0) {
        ImGui::SameLine(0.0f, 0.0f);
      }
      const ToolbarChoiceView &choice = *choices[index];
      const steppenface::Availability &availability =
          choice.action.availability;
      const bool disabled = !availability.enabled || availability.busy;
      const ImVec2 size(
          std::max(Scale(56.0f), ImGui::CalcTextSize(choice.label.c_str()).x +
                                     metrics.spacing.space06),
          metrics.geometry.control_height);
      ImGui::PushID(choice.id.value.c_str());
      ImGui::BeginDisabled(disabled);
      const bool activated =
          ImGui::InvisibleButton("##segment", size, ImGuiButtonFlags_EnableNav);
      interactions.push_back({
          .choice = &choice,
          .minimum = ImGui::GetItemRectMin(),
          .maximum = ImGui::GetItemRectMax(),
          .hovered = ImGui::IsItemHovered(),
          .pressed = ImGui::IsItemActive(),
          .keyboard_focused =
              ImGui::IsItemFocused() && ImGui::GetIO().NavVisible,
          .disabled = disabled,
      });
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        const std::string &message = availability.enabled
                                         ? choice.tooltip
                                         : availability.disabled_reason;
        if (!message.empty()) {
          ShowTooltip(message);
        }
      }
      if (activated && !disabled) {
        Commit(choice.action, callbacks);
      }
      ImGui::PopID();
    }

    const SemanticPalette &palette = CurrentPalette();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    for (std::size_t index = 0; index < interactions.size(); ++index) {
      const ToolbarSegmentInteraction &interaction = interactions[index];
      const bool first = index == 0;
      const bool last = index + 1 == interactions.size();
      const ImDrawFlags corners = first && last ? ImDrawFlags_RoundCornersAll
                                  : first       ? ImDrawFlags_RoundCornersLeft
                                  : last        ? ImDrawFlags_RoundCornersRight
                                                : ImDrawFlags_RoundCornersNone;
      const bool selected = interaction.choice->selected;
      const ColorRgba fill = interaction.disabled
                                 ? palette.control_disabled_fill
                             : selected            ? palette.selection
                             : interaction.pressed ? palette.control_pressed
                             : interaction.hovered ? palette.control_hover
                                                   : palette.surface;
      const ColorRgba foreground = interaction.disabled ? palette.text_disabled
                                   : selected ? palette.focus
                                              : palette.text_secondary;
      draw_list->AddRectFilled(interaction.minimum, interaction.maximum,
                               ImGui::GetColorU32(ToImVec4(fill)),
                               metrics.geometry.surface_radius, corners);
      const ImVec2 label_size =
          ImGui::CalcTextSize(interaction.choice->label.c_str());
      draw_list->AddText(
          ImVec2(std::floor((interaction.minimum.x + interaction.maximum.x -
                             label_size.x) *
                            0.5f),
                 std::floor((interaction.minimum.y + interaction.maximum.y -
                             label_size.y) *
                                0.5f -
                            Scale(1.0f))),
          ImGui::GetColorU32(ToImVec4(foreground)),
          interaction.choice->label.c_str());
      if (selected) {
        draw_list->AddRectFilled(
            ImVec2(interaction.minimum.x + metrics.geometry.border,
                   interaction.maximum.y - Scale(3.0f)),
            ImVec2(interaction.maximum.x - metrics.geometry.border,
                   interaction.maximum.y - metrics.geometry.border),
            ImGui::GetColorU32(ToImVec4(palette.focus)));
      }
      if (interaction.keyboard_focused) {
        draw_list->AddRect(ImVec2(interaction.minimum.x + Scale(3.0f),
                                  interaction.minimum.y + Scale(3.0f)),
                           ImVec2(interaction.maximum.x - Scale(3.0f),
                                  interaction.maximum.y - Scale(3.0f)),
                           ImGui::GetColorU32(ToImVec4(palette.focus)),
                           metrics.geometry.surface_radius, corners,
                           metrics.geometry.focus_ring);
      }
    }
    const ImVec2 group_minimum = interactions.front().minimum;
    const ImVec2 group_maximum = interactions.back().maximum;
    draw_list->AddRect(group_minimum, group_maximum,
                       ImGui::GetColorU32(ToImVec4(palette.border_strong)),
                       metrics.geometry.surface_radius,
                       ImDrawFlags_RoundCornersAll, metrics.geometry.border);
    for (std::size_t index = 1; index < interactions.size(); ++index) {
      const float separator_x = interactions[index].minimum.x;
      draw_list->AddLine(ImVec2(separator_x, group_minimum.y),
                         ImVec2(separator_x, group_maximum.y),
                         ImGui::GetColorU32(ToImVec4(palette.border)),
                         metrics.geometry.border);
    }
    ImGui::PopID();
  }

  void DrawToolbarPopover(const ToolbarPopoverView &popover,
                          const ApplicationChromeCallbacks &callbacks,
                          const bool icon_only,
                          const LayoutMetrics &metrics) const {
    if (!popover.availability.visible) {
      return;
    }
    if (ToolbarButton(popover.id.value, popover.label, popover.icon,
                      popover.tooltip, popover.availability, false, icon_only,
                      CommandVariant::Normal, metrics)) {
      ImGui::OpenPopup(("##popup." + popover.id.value).c_str());
    }
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(Scale(224.0f), 0.0f),
        ImVec2(Scale(320.0f), std::numeric_limits<float>::max()));
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(metrics.spacing.space03, metrics.spacing.space03));
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(metrics.spacing.space03, metrics.spacing.space02));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,
                          ToImVec4(CurrentPalette().application_surface));
    if (ImGui::BeginPopup(("##popup." + popover.id.value).c_str())) {
      for (const ToolbarMenuItemView &item : popover.items) {
        if (item.separator_before) {
          ImGui::Separator();
        }
        const bool enabled = Available(item.action.availability);
        const char *secondary = item.secondary_label.empty()
                                    ? nullptr
                                    : item.secondary_label.c_str();
        if (ImGui::MenuItem(item.label.c_str(), secondary, item.selected,
                            enabled)) {
          Commit(item.action, callbacks);
        }
        if (!enabled &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
            !item.action.availability.disabled_reason.empty()) {
          ShowTooltip(item.action.availability.disabled_reason);
        }
      }
      if (callbacks.draw_field) {
        for (const FieldView &field : popover.fields) {
          ImGui::Separator();
          callbacks.draw_field(field);
        }
      }
      ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
  }

  void DrawToolbarItem(const ToolbarItemView &item,
                       const ApplicationChromeCallbacks &callbacks,
                       const bool icon_only,
                       const LayoutMetrics &metrics) const {
    std::visit(
        [this, &callbacks, icon_only, &metrics](const auto &value) {
          using Item = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<Item, CommandView>) {
            if (ToolbarButton(value.id.value, value.label, value.icon,
                              value.tooltip, value.availability, false,
                              icon_only, value.variant, metrics)) {
              Invoke(value, callbacks);
            }
          } else if constexpr (std::is_same_v<Item, ToolbarSegmentedView>) {
            DrawToolbarSegmented(value, callbacks, metrics);
          } else if constexpr (std::is_same_v<Item, ToolbarActionView>) {
            if (ToolbarButton(value.id.value, value.label, value.icon,
                              value.tooltip, value.action.availability,
                              value.selected, icon_only, CommandVariant::Normal,
                              metrics)) {
              Commit(value.action, callbacks);
            }
          } else if constexpr (std::is_same_v<Item, ToolbarSeparatorView>) {
            const ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(metrics.geometry.border,
                                metrics.geometry.compact_target));
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(cursor.x, cursor.y + metrics.spacing.space02),
                ImVec2(cursor.x, cursor.y + metrics.geometry.compact_target -
                                     metrics.spacing.space02),
                ImGui::GetColorU32(ToImVec4(CurrentPalette().border)),
                metrics.geometry.border);
          } else if constexpr (std::is_same_v<Item, ToolbarPopoverView>) {
            DrawToolbarPopover(value, callbacks, icon_only, metrics);
          }
        },
        item);
  }

  float ToolbarItemWidth(const ToolbarItemView &item, const bool icon_only,
                         const LayoutMetrics &metrics) const {
    return std::visit(
        [icon_only, &metrics](const auto &value) {
          using Item = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<Item, CommandView>) {
            if (!value.availability.visible) {
              return 0.0f;
            }
            return icon_only
                       ? metrics.geometry.control_height
                       : std::max(Scale(56.0f),
                                  ImGui::CalcTextSize(value.label.c_str()).x +
                                      metrics.spacing.space06);
          } else if constexpr (std::is_same_v<Item, ToolbarSegmentedView>) {
            float width = 0.0f;
            for (const ToolbarChoiceView &choice : value.choices) {
              if (choice.action.availability.visible) {
                width += std::max(Scale(56.0f),
                                  ImGui::CalcTextSize(choice.label.c_str()).x +
                                      metrics.spacing.space06);
              }
            }
            return width;
          } else if constexpr (std::is_same_v<Item, ToolbarActionView>) {
            return value.action.availability.visible
                       ? (icon_only
                              ? metrics.geometry.control_height
                              : std::max(
                                    Scale(56.0f),
                                    ImGui::CalcTextSize(value.label.c_str()).x +
                                        metrics.spacing.space06))
                       : 0.0f;
          } else if constexpr (std::is_same_v<Item, ToolbarSeparatorView>) {
            return metrics.geometry.border;
          } else if constexpr (std::is_same_v<Item, ToolbarPopoverView>) {
            return value.availability.visible
                       ? (icon_only
                              ? metrics.geometry.control_height
                              : std::max(
                                    Scale(56.0f),
                                    ImGui::CalcTextSize(value.label.c_str()).x +
                                        metrics.spacing.space06))
                       : 0.0f;
          }
          return 0.0f;
        },
        item);
  }

  void DrawToolbarSequence(const std::vector<ToolbarItemView> &items,
                           const std::size_t begin, const std::size_t end,
                           const ApplicationChromeCallbacks &callbacks,
                           const LayoutMetrics &metrics) const {
    for (std::size_t index = begin; index < end; ++index) {
      if (index > begin) {
        ImGui::SameLine(0.0f, metrics.spacing.space03);
      }
      DrawToolbarItem(items[index], callbacks, false, metrics);
    }
  }

  void DrawContextToolbar(const ContextToolbarView &toolbar,
                          const ApplicationChromeCallbacks &callbacks) const {
    const std::vector<ToolbarItemView> &items = toolbar.items;
    if (items.empty()) {
      return;
    }
    const LayoutMetrics metrics = CurrentLayoutMetrics();
    const auto spacer = std::find_if(
        items.begin(), items.end(), [](const ToolbarItemView &item) {
          return std::holds_alternative<ToolbarSpacerView>(item);
        });
    const std::size_t split =
        static_cast<std::size_t>(std::distance(items.begin(), spacer));
    ImGui::SetCursorPos(
        ImVec2(metrics.spacing.space04, metrics.spacing.space02));
    if (spacer == items.end()) {
      DrawToolbarSequence(items, 0, items.size(), callbacks, metrics);
      return;
    }
    DrawToolbarSequence(items, 0, split, callbacks, metrics);
    float right_width = 0.0f;
    std::size_t right_count = 0;
    for (std::size_t index = split + 1; index < items.size(); ++index) {
      const float item_width = ToolbarItemWidth(items[index], false, metrics);
      if (item_width > 0.0f) {
        right_width += item_width;
        ++right_count;
      }
    }
    if (right_count > 1) {
      right_width +=
          static_cast<float>(right_count - 1) * metrics.spacing.space03;
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(
        std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - right_width -
                                             metrics.spacing.space04));
    DrawToolbarSequence(items, split + 1, items.size(), callbacks, metrics);
  }
};

ApplicationChrome::ApplicationChrome(UiAssetAtlas &assets)
    : impl_(std::make_unique<Impl>(assets)) {}

ApplicationChrome::~ApplicationChrome() = default;
ApplicationChrome::ApplicationChrome(ApplicationChrome &&) noexcept = default;
ApplicationChrome &
ApplicationChrome::operator=(ApplicationChrome &&) noexcept = default;

void ApplicationChrome::DrawApplicationBar(
    const ApplicationBarView &view, const ChromeLayoutState &layout,
    const ApplicationChromeCallbacks &callbacks,
    const ApplicationBarHost host) {
  impl_->DrawApplicationBar(view, layout, callbacks, host);
}

void ApplicationChrome::DrawContextToolbar(
    const ContextToolbarView &view,
    const ApplicationChromeCallbacks &callbacks) {
  impl_->DrawContextToolbar(view, callbacks);
}

} // namespace fancy_ui::detail
