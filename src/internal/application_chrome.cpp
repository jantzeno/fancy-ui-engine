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

  void
  DrawWorkspaceSwitcher(const ApplicationBarView &bar,
                        const ApplicationChromeCallbacks &callbacks,
                        const LayoutMetrics &metrics,
                        const std::span<const InteractionPreview> previews = {},
                        const float logical_segment_width = 72.0f) const {
    const ToolbarSegmentedView segmented{
        .id = {.value = "workspace-switcher"},
        .choices =
            {
                {.id = {.value = "workspace.3d"},
                 .label = "3D",
                 .selected = bar.active_workspace == WorkspaceKind::Model3d,
                 .action = {.field = {.value = "workspace"},
                            .value = std::string("model")}},
                {.id = {.value = "workspace.canvas"},
                 .label = "Canvas",
                 .selected = bar.active_workspace == WorkspaceKind::Canvas,
                 .action = {.field = {.value = "workspace"},
                            .value = std::string("canvas")}},
            },
    };
    ApplicationChromeCallbacks workspace_callbacks = callbacks;
    workspace_callbacks.commit_action = [activate_workspace =
                                             callbacks.activate_workspace](
                                            const ControlActionView &action) {
      if (!activate_workspace) {
        return;
      }
      const std::string *workspace = std::get_if<std::string>(&action.value);
      if (workspace == nullptr) {
        return;
      }
      if (*workspace == "model") {
        activate_workspace(WorkspaceKind::Model3d);
      } else if (*workspace == "canvas") {
        activate_workspace(WorkspaceKind::Canvas);
      }
    };
    DrawToolbarSegmented(segmented, workspace_callbacks, metrics, previews,
                         logical_segment_width * 2.0f);
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
                     const bool has_popup, const CommandVariant variant,
                     const LayoutMetrics &metrics) const {
    if (!availability.visible) {
      return false;
    }
    const SemanticPalette &palette = CurrentPalette();
    const bool disabled = !availability.enabled || availability.busy;
    const float disclosure_width =
        !icon_only && has_popup ? Scale(16.0f) : 0.0f;
    const ImVec2 size =
        icon_only
            ? ImVec2(metrics.geometry.control_height,
                     metrics.geometry.control_height)
            : ImVec2(std::max(Scale(56.0f),
                              ImGui::CalcTextSize(label.c_str()).x +
                                  metrics.spacing.space06 + disclosure_width),
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
      const float label_maximum_x = maximum.x - disclosure_width;
      draw_list->AddText(
          ImVec2(
              std::floor((minimum.x + label_maximum_x - label_size.x) * 0.5f),
              std::floor((minimum.y + maximum.y - label_size.y) * 0.5f -
                         Scale(1.0f))),
          ImGui::GetColorU32(ToImVec4(foreground)), label.c_str());
      if (has_popup) {
        const float center_x = maximum.x - Scale(10.0f);
        const float center_y = (minimum.y + maximum.y) * 0.5f;
        draw_list->AddTriangleFilled(
            ImVec2(center_x - Scale(3.0f), center_y - Scale(1.5f)),
            ImVec2(center_x + Scale(3.0f), center_y - Scale(1.5f)),
            ImVec2(center_x, center_y + Scale(2.0f)),
            ImGui::GetColorU32(ToImVec4(foreground)));
      }
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

  void
  DrawToolbarSegmented(const ToolbarSegmentedView &segmented,
                       const ApplicationChromeCallbacks &callbacks,
                       const LayoutMetrics &metrics,
                       const std::span<const InteractionPreview> previews = {},
                       const float logical_width = 0.0f) const {
    std::vector<const ToolbarChoiceView *> choices;
    for (const ToolbarChoiceView &choice : segmented.choices) {
      if (choice.action.availability.visible) {
        choices.push_back(&choice);
      }
    }
    if (choices.empty()) {
      return;
    }

    ImGui::PushFont(nullptr, Scale(21.0f));
    ImGui::PushID(segmented.id.value.c_str());
    std::vector<ToolbarSegmentInteraction> interactions;
    const float equal_width =
        logical_width > 0.0f
            ? Scale(logical_width) / static_cast<float>(choices.size())
            : 0.0f;
    for (std::size_t index = 0; index < choices.size(); ++index) {
      if (index > 0) {
        ImGui::SameLine(0.0f, 0.0f);
      }
      const ToolbarChoiceView &choice = *choices[index];
      const steppenface::Availability &availability =
          choice.action.availability;
      const bool disabled = !availability.enabled || availability.busy;
      const ImVec2 size(
          equal_width > 0.0f
              ? equal_width
              : std::max(Scale(56.0f),
                         ImGui::CalcTextSize(choice.label.c_str()).x +
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
      if (index < previews.size()) {
        ToolbarSegmentInteraction &interaction = interactions.back();
        interaction.hovered = previews[index] == InteractionPreview::Hovered ||
                              previews[index] == InteractionPreview::Pressed;
        interaction.pressed = previews[index] == InteractionPreview::Pressed;
        interaction.keyboard_focused =
            previews[index] == InteractionPreview::Focused;
      }
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
    const auto corners_for = [&interactions](const std::size_t index) {
      if (interactions.size() == 1) {
        return ImDrawFlags_RoundCornersAll;
      }
      if (index == 0) {
        return ImDrawFlags_RoundCornersLeft;
      }
      return index + 1 == interactions.size() ? ImDrawFlags_RoundCornersRight
                                              : ImDrawFlags_RoundCornersNone;
    };
    const auto colors_for =
        [&palette](const ToolbarSegmentInteraction &interaction) {
          ControlColors colors{
              .fill = palette.surface_raised,
              .border = palette.border_strong,
              .text = palette.text_primary,
          };
          if (interaction.choice->selected) {
            colors.fill = palette.selection;
            colors.border = palette.focus;
          }
          if (interaction.hovered) {
            colors.fill = palette.control_hover;
          }
          if (interaction.pressed) {
            colors.fill = palette.control_pressed;
          }
          if (interaction.disabled) {
            colors.fill = palette.control_disabled_fill;
            colors.border = palette.border;
            colors.text = palette.text_disabled;
          }
          return colors;
        };
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    for (std::size_t index = 0; index < interactions.size(); ++index) {
      const ToolbarSegmentInteraction &interaction = interactions[index];
      const bool selected = interaction.choice->selected;
      const ControlColors colors = colors_for(interaction);
      draw_list->AddRectFilled(interaction.minimum, interaction.maximum,
                               ImGui::GetColorU32(ToImVec4(colors.fill)),
                               metrics.geometry.control_radius,
                               corners_for(index));
      const ImVec2 label_size =
          ImGui::CalcTextSize(interaction.choice->label.c_str());
      draw_list->AddText(
          ImVec2(std::floor((interaction.minimum.x + interaction.maximum.x -
                             label_size.x) *
                            0.5f),
                 std::floor((interaction.minimum.y + interaction.maximum.y -
                             label_size.y) *
                            0.5f)),
          ImGui::GetColorU32(ToImVec4(colors.text)),
          interaction.choice->label.c_str());
      if (selected) {
        draw_list->AddRectFilled(
            ImVec2(interaction.minimum.x + metrics.geometry.border,
                   interaction.maximum.y - Scale(3.0f)),
            ImVec2(interaction.maximum.x - metrics.geometry.border,
                   interaction.maximum.y - metrics.geometry.border),
            ImGui::GetColorU32(ToImVec4(palette.focus)));
      }
    }
    const auto draw_border = [&](const std::size_t index) {
      const ToolbarSegmentInteraction &interaction = interactions[index];
      draw_list->AddRect(
          interaction.minimum, interaction.maximum,
          ImGui::GetColorU32(ToImVec4(colors_for(interaction).border)),
          metrics.geometry.control_radius, corners_for(index),
          metrics.geometry.border);
    };
    for (std::size_t index = 0; index < interactions.size(); ++index) {
      if (!interactions[index].choice->selected) {
        draw_border(index);
      }
    }
    for (std::size_t index = 0; index < interactions.size(); ++index) {
      if (interactions[index].choice->selected) {
        draw_border(index);
      }
    }
    for (std::size_t index = 0; index < interactions.size(); ++index) {
      const ToolbarSegmentInteraction &interaction = interactions[index];
      if (!interaction.keyboard_focused) {
        continue;
      }
      draw_list->AddRect(ImVec2(interaction.minimum.x + Scale(3.0f),
                                interaction.minimum.y + Scale(3.0f)),
                         ImVec2(interaction.maximum.x - Scale(3.0f),
                                interaction.maximum.y - Scale(3.0f)),
                         ImGui::GetColorU32(ToImVec4(palette.focus)),
                         metrics.geometry.control_radius, corners_for(index),
                         metrics.geometry.focus_ring);
    }
    ImGui::PopID();
    ImGui::PopFont();
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
                      true, CommandVariant::Normal, metrics)) {
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
      for (const ToolbarPopoverItemView &item : popover.items) {
        std::visit(
            [this, &callbacks](const auto &value) {
              using Item = std::decay_t<decltype(value)>;
              if constexpr (std::is_same_v<Item, CommandView>) {
                if (!value.availability.visible) {
                  return;
                }
                ImGui::PushID(value.id.value.c_str());
                const bool enabled = Available(value.availability);
                const char *shortcut =
                    value.shortcut.empty() ? nullptr : value.shortcut.c_str();
                if (ImGui::MenuItem(value.label.c_str(), shortcut, false,
                                    enabled)) {
                  Invoke(value, callbacks);
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                  const std::string &message =
                      enabled ? value.tooltip
                              : value.availability.disabled_reason;
                  if (!message.empty()) {
                    ShowTooltip(message);
                  }
                }
                ImGui::PopID();
              } else {
                if (!value.action.availability.visible) {
                  return;
                }
                if (value.separator_before) {
                  ImGui::Separator();
                }
                ImGui::PushID(value.id.value.c_str());
                const bool enabled = Available(value.action.availability);
                const char *secondary = value.secondary_label.empty()
                                            ? nullptr
                                            : value.secondary_label.c_str();
                if (ImGui::MenuItem(value.label.c_str(), secondary,
                                    value.selected, enabled)) {
                  Commit(value.action, callbacks);
                }
                if (!enabled &&
                    ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
                    !value.action.availability.disabled_reason.empty()) {
                  ShowTooltip(value.action.availability.disabled_reason);
                }
                ImGui::PopID();
              }
            },
            item);
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
                              icon_only, false, value.variant, metrics)) {
              Invoke(value, callbacks);
            }
          } else if constexpr (std::is_same_v<Item, ToolbarSegmentedView>) {
            DrawToolbarSegmented(value, callbacks, metrics);
          } else if constexpr (std::is_same_v<Item, ToolbarActionView>) {
            if (ToolbarButton(value.id.value, value.label, value.icon,
                              value.tooltip, value.action.availability,
                              value.selected, icon_only, false,
                              CommandVariant::Normal, metrics)) {
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
                                        metrics.spacing.space06 + Scale(16.0f)))
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

void ApplicationChrome::DrawWorkspaceSwitcher(
    const ApplicationBarView &view, const ApplicationChromeCallbacks &callbacks,
    const std::span<const InteractionPreview> previews,
    const float logical_segment_width) {
  impl_->DrawWorkspaceSwitcher(view, callbacks, CurrentLayoutMetrics(),
                               previews, logical_segment_width);
}

void ApplicationChrome::DrawToolbarSegmented(
    const ToolbarSegmentedView &view,
    const ApplicationChromeCallbacks &callbacks,
    const std::span<const InteractionPreview> previews,
    const float logical_width) {
  impl_->DrawToolbarSegmented(view, callbacks, CurrentLayoutMetrics(), previews,
                              logical_width);
}

void ApplicationChrome::DrawContextToolbar(
    const ContextToolbarView &view,
    const ApplicationChromeCallbacks &callbacks) {
  impl_->DrawContextToolbar(view, callbacks);
}

} // namespace fancy_ui::detail
