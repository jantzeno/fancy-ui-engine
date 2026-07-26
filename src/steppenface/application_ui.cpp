#include "fancy_ui/steppenface/application_ui.hpp"

#include "fancy_ui/components/button.hpp"
#include "fancy_ui/components/checkbox.hpp"
#include "fancy_ui/components/data_display.hpp"
#include "fancy_ui/components/navigation.hpp"
#include "fancy_ui/shell/application.hpp"
#include "fancy_ui/theme.hpp"

#include "ui/im2d_canvas_widget.h"

#include <imgui.h>
#include <lunasvg.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fancy_ui::steppenface {

namespace {

SemanticStatus ToStatus(const SemanticTone tone) {
  switch (tone) {
  case SemanticTone::Information:
    return SemanticStatus::Information;
  case SemanticTone::Success:
    return SemanticStatus::Success;
  case SemanticTone::Warning:
    return SemanticStatus::Warning;
  case SemanticTone::Failure:
    return SemanticStatus::Failure;
  case SemanticTone::Neutral:
    return SemanticStatus::Neutral;
  }
  return SemanticStatus::Neutral;
}

fancy_ui::Availability ToAvailability(const Availability &availability) {
  return {
      .enabled = availability.enabled,
      .busy = availability.busy,
      .reason = availability.disabled_reason,
  };
}

ButtonVariant ToButtonVariant(const CommandVariant variant) {
  switch (variant) {
  case CommandVariant::Primary:
    return ButtonVariant::Primary;
  case CommandVariant::Destructive:
    return ButtonVariant::Destructive;
  case CommandVariant::Tertiary:
    return ButtonVariant::Tertiary;
  case CommandVariant::Normal:
    return ButtonVariant::Secondary;
  }
  return ButtonVariant::Secondary;
}

ImTextureID ToImTextureId(const TextureHandle handle) {
  return static_cast<ImTextureID>(handle.value);
}

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

bool MatchesQuery(const TreeRowView &row, const std::string &query) {
  if (query.empty()) {
    return true;
  }
  const auto contains_case_insensitive = [&query](const std::string &value) {
    return std::search(value.begin(), value.end(), query.begin(), query.end(),
                       [](const char left, const char right) {
                         return std::tolower(
                                    static_cast<unsigned char>(left)) ==
                                std::tolower(static_cast<unsigned char>(right));
                       }) != value.end();
  };
  return contains_case_insensitive(row.label) ||
         contains_case_insensitive(row.secondary_label);
}

} // namespace

class ApplicationUi::Impl {
public:
  static constexpr float kApplicationBarHeight = 40.0f;
  static constexpr float kApplicationBarControlHeight = 32.0f;
  static constexpr float kApplicationMenuFontSize = 18.0f;
  static constexpr float kWorkspaceButtonWidth = 72.0f;
  static constexpr float kMenuWidth = 264.0f;
  static constexpr float kMenuTriggerRounding = 4.0f;
  static constexpr float kToolbarControlHeight = 32.0f;
  static constexpr float kToolbarControlRounding = 4.0f;
  static constexpr float kToolbarLabelOffsetY = -1.0f;

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

  struct PendingIcon {
    std::string name;
    lunasvg::Bitmap bitmap;
  };

  SessionState session;
  ImFont *regular_font = nullptr;
  ImFont *bold_font = nullptr;
  ImFont *mono_font = nullptr;
  float application_menu_font_size = kApplicationMenuFontSize;
  std::unordered_map<std::string, ImFontAtlasRectId> icon_rects;
  std::vector<PendingIcon> pending_icons;
  std::vector<UiIntent> intents;
  bool navigation_changed = false;
  bool layout_changed = false;

  void InstallPendingIcons() {
    ImFontAtlas *atlas = ImGui::GetIO().Fonts;
    if (pending_icons.empty() || !atlas->RendererHasTextures ||
        atlas->TexData == nullptr || atlas->TexData->Pixels == nullptr) {
      return;
    }

    for (const PendingIcon &icon : pending_icons) {
      ImFontAtlasRect rect;
      const ImFontAtlasRectId rect_id = atlas->AddCustomRect(
          icon.bitmap.width(), icon.bitmap.height(), &rect);
      if (rect_id == ImFontAtlasRectId_Invalid) {
        continue;
      }

      ImTextureData *texture = atlas->TexData;
      if (texture == nullptr || texture->Pixels == nullptr ||
          static_cast<int>(rect.x) + icon.bitmap.width() > texture->Width ||
          static_cast<int>(rect.y) + icon.bitmap.height() > texture->Height) {
        atlas->RemoveCustomRect(rect_id);
        continue;
      }

      for (int y = 0; y < icon.bitmap.height(); ++y) {
        const unsigned char *source =
            icon.bitmap.data() + y * icon.bitmap.stride();
        unsigned char *destination =
            texture->Pixels + ((static_cast<int>(rect.y) + y) * texture->Width +
                               static_cast<int>(rect.x)) *
                                  texture->BytesPerPixel;
        for (int x = 0; x < icon.bitmap.width(); ++x) {
          if (texture->BytesPerPixel == 4) {
            destination[x * 4 + 0] = 255;
            destination[x * 4 + 1] = 255;
            destination[x * 4 + 2] = 255;
            destination[x * 4 + 3] = source[x * 4 + 3];
          } else {
            destination[x] = source[x * 4 + 3];
          }
        }
      }
      icon_rects.emplace(icon.name, rect_id);
    }
    pending_icons.clear();
  }

  void EmitCommand(const std::uint64_t revision, const CommandView &command) {
    if (command.availability.visible && command.availability.enabled &&
        !command.availability.busy) {
      intents.emplace_back(
          InvokeCommand{.revision = revision, .command = command.command});
    }
  }

  void EmitEdit(const std::uint64_t revision, const ControlActionView &action) {
    if (!action.availability.visible || !action.availability.enabled ||
        action.availability.busy) {
      return;
    }
    if (action.field.value == "session.model-grid-target") {
      if (const std::string *target = std::get_if<std::string>(&action.value)) {
        session.model_grid_target = {.value = *target};
      }
      return;
    }
    intents.emplace_back(EditField{
        .revision = revision,
        .field = action.field,
        .value = action.value,
        .target = action.target,
        .phase = EditPhase::Commit,
    });
  }

  void DrawCommandButton(const std::uint64_t revision,
                         const CommandView &command, const bool compact) {
    if (!command.availability.visible) {
      return;
    }
    const ButtonResult result = Button({
        .id = command.id.value,
        .label = command.label,
        .tooltip = command.tooltip,
        .variant = ToButtonVariant(command.variant),
        .availability = ToAvailability(command.availability),
        .size = {compact ? 0.0f : -1.0f, 32.0f},
    });
    if (result.activated) {
      EmitCommand(revision, command);
    }
  }

  void ActivateWorkspace(const WorkspaceKind workspace) {
    if (WorkspaceForDestination(session.active_destination) == workspace) {
      return;
    }
    session.ActivateWorkspace(workspace);
    navigation_changed = true;
  }

  void DrawMenuSeparator() { ImGui::Separator(); }

  void DrawMenuItems(const ApplicationView &view,
                     const std::vector<MenuItemView> &items) {
    for (const MenuItemView &item : items) {
      switch (item.kind) {
      case MenuItemKind::Separator:
        DrawMenuSeparator();
        break;
      case MenuItemKind::Submenu:
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(kMenuWidth, 0.0f),
            ImVec2(kMenuWidth, std::numeric_limits<float>::max()));
        if (ImGui::BeginMenu(item.label.c_str())) {
          DrawMenuItems(view, item.children);
          ImGui::EndMenu();
        }
        break;
      case MenuItemKind::Workspace: {
        if (!item.workspace.has_value()) {
          break;
        }
        const bool selected =
            *item.workspace == view.application_bar.active_workspace;
        if (ImGui::MenuItem(item.label.c_str(), nullptr, selected, true)) {
          ActivateWorkspace(*item.workspace);
        }
        break;
      }
      case MenuItemKind::Command: {
        if (!item.command.has_value()) {
          break;
        }
        const CommandView &command = *item.command;
        if (!command.availability.visible) {
          break;
        }
        const bool enabled =
            command.availability.enabled && !command.availability.busy;
        const char *shortcut =
            command.shortcut.empty() ? nullptr : command.shortcut.c_str();
        if (ImGui::MenuItem(command.label.c_str(), shortcut, false, enabled)) {
          EmitCommand(view.revision, command);
        }
        if (!enabled &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
            !command.availability.disabled_reason.empty()) {
          ImGui::SetTooltip("%s", command.availability.disabled_reason.c_str());
        }
        break;
      }
      }
    }
  }

  [[nodiscard]] WorkspaceSegmentInteraction
  CaptureWorkspaceSegment(const WorkspaceKind workspace, const char *label) {
    ImGui::PushID(workspace == WorkspaceKind::Model3d ? "workspace.3d"
                                                      : "workspace.canvas");
    if (ImGui::InvisibleButton(
            "##segment",
            ImVec2(kWorkspaceButtonWidth, kApplicationBarControlHeight),
            ImGuiButtonFlags_EnableNav)) {
      ActivateWorkspace(workspace);
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

  void DrawWorkspaceSwitcher(const ApplicationView &view) {
    const SemanticPalette &palette = CurrentPalette();
    const ImVec2 minimum = ImGui::GetCursorScreenPos();
    const WorkspaceSegmentInteraction model =
        CaptureWorkspaceSegment(WorkspaceKind::Model3d, "3D");
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetCursorScreenPos(
        ImVec2(minimum.x + kWorkspaceButtonWidth, minimum.y));
    const WorkspaceSegmentInteraction canvas =
        CaptureWorkspaceSegment(WorkspaceKind::Canvas, "Canvas");
    const ImVec2 maximum(minimum.x + kWorkspaceButtonWidth * 2.0f,
                         minimum.y + kApplicationBarControlHeight);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const std::array<WorkspaceSegmentInteraction, 2> interactions = {model,
                                                                     canvas};
    for (std::size_t index = 0; index < interactions.size(); ++index) {
      const WorkspaceSegmentInteraction &interaction = interactions[index];
      const WorkspaceKind workspace =
          index == 0 ? WorkspaceKind::Model3d : WorkspaceKind::Canvas;
      const bool selected = workspace == view.application_bar.active_workspace;
      const ColorRgba fill = selected              ? palette.selection
                             : interaction.pressed ? palette.control_pressed
                             : interaction.hovered ? palette.control_hover
                                                   : palette.surface;
      const ImVec2 segment_minimum(minimum.x + kWorkspaceButtonWidth *
                                                   static_cast<float>(index),
                                   minimum.y);
      const ImVec2 segment_maximum(segment_minimum.x + kWorkspaceButtonWidth,
                                   maximum.y);
      const ImDrawFlags corners = index == 0 ? ImDrawFlags_RoundCornersLeft
                                             : ImDrawFlags_RoundCornersRight;
      draw_list->AddRectFilled(segment_minimum, segment_maximum,
                               ImGui::GetColorU32(ToImVec4(fill)), 4.0f,
                               corners);
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
            ImVec2(segment_minimum.x + 1.0f, maximum.y - 3.0f),
            ImVec2(segment_maximum.x - 1.0f, maximum.y - 1.0f),
            ImGui::GetColorU32(ToImVec4(palette.focus)));
      }
      if (interaction.keyboard_focused) {
        draw_list->AddRect(
            ImVec2(segment_minimum.x + 3.0f, segment_minimum.y + 3.0f),
            ImVec2(segment_maximum.x - 3.0f, segment_maximum.y - 3.0f),
            ImGui::GetColorU32(ToImVec4(palette.focus)), 2.0f, corners, 2.0f);
      }
    }

    draw_list->AddRect(minimum, maximum,
                       ImGui::GetColorU32(ToImVec4(palette.border)), 4.0f,
                       ImDrawFlags_RoundCornersAll, 1.0f);
    const float separator_x = minimum.x + kWorkspaceButtonWidth;
    draw_list->AddLine(ImVec2(separator_x, minimum.y),
                       ImVec2(separator_x, maximum.y),
                       ImGui::GetColorU32(ToImVec4(palette.border)));
  }

  [[nodiscard]] bool BeginApplicationMenu(const char *label) {
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
          kMenuTriggerRounding);
    }
    draw_list->ChannelsMerge();
    return open;
  }

  void DrawApplicationBar(const ApplicationView &view) {
    ImGui::PushFont(regular_font, application_menu_font_size);
    const float vertical_padding =
        std::max(0.0f, (kApplicationBarHeight - ImGui::GetFontSize()) * 0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(12.0f, vertical_padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 0.0f));
    const bool open = ImGui::BeginMainMenuBar();
    ImGui::PopStyleVar(2);
    if (!open) {
      ImGui::PopFont();
      return;
    }

    const SemanticPalette &palette = CurrentPalette();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 7.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(8.0f, std::max(0.0f, kApplicationBarControlHeight -
                                        ImGui::GetFontSize())));
    ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(palette.border_strong));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,
                          ToImVec4(palette.application_surface));
    for (const ApplicationMenuView &menu : view.application_bar.menus) {
      ImGui::SetNextWindowSizeConstraints(
          ImVec2(kMenuWidth, 0.0f),
          ImVec2(kMenuWidth, std::numeric_limits<float>::max()));
      if (BeginApplicationMenu(menu.label.c_str())) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
        DrawMenuItems(view, menu.items);
        ImGui::PopStyleVar();
        ImGui::EndMenu();
      }
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    ImGui::PopFont();

    const float switcher_x = ImGui::GetCursorPosX();
    ImGui::SetCursorPos(ImVec2(switcher_x, 4.0f));
    DrawWorkspaceSwitcher(view);

    if (view.application_bar.document_dirty &&
        !view.application_bar.dirty_label.empty()) {
      const ImVec2 switcher_minimum = ImGui::GetItemRectMin();
      const ImVec2 switcher_maximum = ImGui::GetItemRectMax();
      const float center_y = (switcher_minimum.y + switcher_maximum.y) * 0.5f;
      const float marker_x = switcher_maximum.x + 16.0f;
      const ImVec2 text_size =
          ImGui::CalcTextSize(view.application_bar.dirty_label.c_str());
      ImGui::GetWindowDrawList()->AddCircleFilled(
          ImVec2(marker_x, center_y), 4.0f,
          ImGui::GetColorU32(ToImVec4(palette.warning)));
      ImGui::GetWindowDrawList()->AddText(
          ImVec2(marker_x + 12.0f, center_y - text_size.y * 0.5f),
          ImGui::GetColorU32(ToImVec4(palette.warning)),
          view.application_bar.dirty_label.c_str());
    }

    const ImVec2 window_position = ImGui::GetWindowPos();
    const ImVec2 window_size = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(window_position.x,
               window_position.y + kApplicationBarHeight - 1.0f),
        ImVec2(window_position.x + window_size.x,
               window_position.y + kApplicationBarHeight - 1.0f),
        ImGui::GetColorU32(ToImVec4(palette.border)));

    ImGui::EndMainMenuBar();
  }

  void DrawToolbarIcon(const std::string &icon, const ImVec2 minimum,
                       const ImVec2 maximum, const ImU32 color) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 center((minimum.x + maximum.x) * 0.5f,
                        (minimum.y + maximum.y) * 0.5f);
    const float left = center.x - 7.0f;
    const float right = center.x + 7.0f;
    const float top = center.y - 7.0f;
    const float bottom = center.y + 7.0f;
    if (const auto found = icon_rects.find(icon); found != icon_rects.end()) {
      ImFontAtlasRect rect;
      if (ImGui::GetIO().Fonts->GetCustomRect(found->second, &rect)) {
        draw_list->AddImage(ImGui::GetIO().Fonts->TexRef,
                            ImVec2(center.x - 8.0f, center.y - 8.0f),
                            ImVec2(center.x + 8.0f, center.y + 8.0f), rect.uv0,
                            rect.uv1, color);
        return;
      }
    }
    if (icon == "fit") {
      draw_list->AddRect(ImVec2(left + 2.0f, top + 3.0f),
                         ImVec2(right - 2.0f, bottom - 3.0f), color, 1.5f,
                         ImDrawFlags_RoundCornersAll, 1.5f);
      draw_list->AddLine(ImVec2(left, top), ImVec2(left + 5.0f, top), color,
                         1.5f);
      draw_list->AddLine(ImVec2(left, top), ImVec2(left, top + 5.0f), color,
                         1.5f);
      draw_list->AddLine(ImVec2(right, bottom), ImVec2(right - 5.0f, bottom),
                         color, 1.5f);
      draw_list->AddLine(ImVec2(right, bottom), ImVec2(right, bottom - 5.0f),
                         color, 1.5f);
    } else if (icon == "focus") {
      draw_list->AddCircle(center, 4.0f, color, 0, 1.5f);
      draw_list->AddLine(ImVec2(center.x, top), ImVec2(center.x, top + 4.0f),
                         color, 1.5f);
      draw_list->AddLine(ImVec2(center.x, bottom - 4.0f),
                         ImVec2(center.x, bottom), color, 1.5f);
      draw_list->AddLine(ImVec2(left, center.y), ImVec2(left + 4.0f, center.y),
                         color, 1.5f);
      draw_list->AddLine(ImVec2(right - 4.0f, center.y),
                         ImVec2(right, center.y), color, 1.5f);
    } else if (icon == "view") {
      const ImVec2 points[] = {
          ImVec2(center.x, top),          ImVec2(right, center.y - 3.0f),
          ImVec2(right, center.y + 5.0f), ImVec2(center.x, bottom),
          ImVec2(left, center.y + 5.0f),  ImVec2(left, center.y - 3.0f),
      };
      draw_list->AddPolyline(points, 6, color, ImDrawFlags_Closed, 1.5f);
      draw_list->AddLine(ImVec2(left, center.y - 3.0f), center, color, 1.5f);
      draw_list->AddLine(ImVec2(right, center.y - 3.0f), center, color, 1.5f);
      draw_list->AddLine(center, ImVec2(center.x, bottom), color, 1.5f);
    } else if (icon == "visibility" || icon == "eye") {
      draw_list->AddBezierCubic(
          ImVec2(left, center.y), ImVec2(center.x - 3.0f, top),
          ImVec2(center.x + 3.0f, top), ImVec2(right, center.y), color, 1.5f);
      draw_list->AddBezierCubic(
          ImVec2(right, center.y), ImVec2(center.x + 3.0f, bottom),
          ImVec2(center.x - 3.0f, bottom), ImVec2(left, center.y), color, 1.5f);
      draw_list->AddCircleFilled(center, 2.5f, color);
    } else if (icon == "orbit-locked" || icon == "orbit-unlocked") {
      draw_list->AddRect(ImVec2(center.x - 5.0f, center.y - 1.0f),
                         ImVec2(center.x + 5.0f, bottom), color, 1.5f, 0, 1.5f);
      draw_list->PathClear();
      draw_list->PathArcTo(ImVec2(center.x, center.y - 1.0f), 4.0f, 3.1415927f,
                           icon == "orbit-locked" ? 6.2831853f : 5.45f);
      draw_list->PathStroke(color, 0, 1.5f);
    } else if (icon == "send") {
      draw_list->AddLine(ImVec2(left, center.y), ImVec2(right, center.y), color,
                         1.5f);
      draw_list->AddLine(ImVec2(right - 5.0f, top + 3.0f),
                         ImVec2(right, center.y), color, 1.5f);
      draw_list->AddLine(ImVec2(right - 5.0f, bottom - 3.0f),
                         ImVec2(right, center.y), color, 1.5f);
    } else {
      const std::string fallback = icon.empty() ? "•" : icon.substr(0, 1);
      const ImVec2 size = ImGui::CalcTextSize(fallback.c_str());
      draw_list->AddText(
          ImVec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f), color,
          fallback.c_str());
    }
  }

  bool ToolbarButton(const std::string &id, const std::string &label,
                     const std::string &icon, const std::string &tooltip,
                     const Availability &availability, const bool selected,
                     const bool icon_only,
                     const CommandVariant variant = CommandVariant::Normal) {
    if (!availability.visible) {
      return false;
    }
    const SemanticPalette &palette = CurrentPalette();
    const bool disabled = !availability.enabled || availability.busy;
    const ImVec2 size =
        icon_only
            ? ImVec2(kToolbarControlHeight, kToolbarControlHeight)
            : ImVec2(
                  std::max(56.0f, ImGui::CalcTextSize(label.c_str()).x + 24.0f),
                  kToolbarControlHeight);

    ImGui::PushID(id.c_str());
    ImGui::BeginDisabled(disabled);
    const bool activated =
        ImGui::InvisibleButton("##button", size, ImGuiButtonFlags_EnableNav);
    const bool hovered = ImGui::IsItemHovered();
    const bool pressed = ImGui::IsItemActive();
    const bool keyboard_focused =
        ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    ImGui::EndDisabled();

    ColorRgba fill = palette.control;
    ColorRgba border = icon_only ? palette.border : palette.border_strong;
    ColorRgba foreground = palette.text_primary;
    if (disabled) {
      fill = palette.surface;
      border = palette.border;
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
                             kToolbarControlRounding);
    draw_list->AddRect(minimum, maximum, ImGui::GetColorU32(ToImVec4(border)),
                       kToolbarControlRounding);
    if (icon_only) {
      DrawToolbarIcon(icon, minimum, maximum,
                      ImGui::GetColorU32(ToImVec4(foreground)));
    } else {
      const ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
      draw_list->AddText(
          ImVec2(std::floor((minimum.x + maximum.x - label_size.x) * 0.5f),
                 std::floor((minimum.y + maximum.y - label_size.y) * 0.5f +
                            kToolbarLabelOffsetY)),
          ImGui::GetColorU32(ToImVec4(foreground)), label.c_str());
    }
    if (keyboard_focused) {
      draw_list->AddRect(ImVec2(minimum.x + 3.0f, minimum.y + 3.0f),
                         ImVec2(maximum.x - 3.0f, maximum.y - 3.0f),
                         ImGui::GetColorU32(ToImVec4(palette.focus)), 2.0f,
                         ImDrawFlags_RoundCornersAll, 2.0f);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      const std::string &message =
          availability.enabled ? tooltip : availability.disabled_reason;
      if (!message.empty()) {
        ImGui::SetTooltip("%s", message.c_str());
      }
    }
    ImGui::PopID();
    return activated && availability.enabled && !availability.busy;
  }

  void DrawToolbarSegmented(const ApplicationView &view,
                            const ToolbarSegmentedView &segmented) {
    std::vector<const ToolbarChoiceView *> choices;
    choices.reserve(segmented.choices.size());
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
    interactions.reserve(choices.size());
    for (std::size_t index = 0; index < choices.size(); ++index) {
      if (index > 0) {
        ImGui::SameLine(0.0f, 0.0f);
      }
      const ToolbarChoiceView &choice = *choices[index];
      const Availability &availability = choice.action.availability;
      const bool disabled = !availability.enabled || availability.busy;
      const ImVec2 size(
          std::max(56.0f, ImGui::CalcTextSize(choice.label.c_str()).x + 24.0f),
          kToolbarControlHeight);

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
          ImGui::SetTooltip("%s", message.c_str());
        }
      }
      if (activated && availability.enabled && !availability.busy) {
        EmitEdit(view.revision, choice.action);
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
      const ColorRgba fill = interaction.disabled  ? palette.surface
                             : selected            ? palette.selection
                             : interaction.pressed ? palette.control_pressed
                             : interaction.hovered ? palette.control_hover
                                                   : palette.surface;
      const ColorRgba foreground = interaction.disabled ? palette.text_disabled
                                   : selected ? palette.focus
                                              : palette.text_secondary;
      draw_list->AddRectFilled(interaction.minimum, interaction.maximum,
                               ImGui::GetColorU32(ToImVec4(fill)),
                               kToolbarControlRounding, corners);

      const ImVec2 label_size =
          ImGui::CalcTextSize(interaction.choice->label.c_str());
      draw_list->AddText(
          ImVec2(std::floor((interaction.minimum.x + interaction.maximum.x -
                             label_size.x) *
                            0.5f),
                 std::floor((interaction.minimum.y + interaction.maximum.y -
                             label_size.y) *
                                0.5f +
                            kToolbarLabelOffsetY)),
          ImGui::GetColorU32(ToImVec4(foreground)),
          interaction.choice->label.c_str());
      if (selected) {
        draw_list->AddRectFilled(
            ImVec2(interaction.minimum.x + 1.0f, interaction.maximum.y - 3.0f),
            ImVec2(interaction.maximum.x - 1.0f, interaction.maximum.y - 1.0f),
            ImGui::GetColorU32(ToImVec4(palette.focus)));
      }
      if (interaction.keyboard_focused) {
        draw_list->AddRect(
            ImVec2(interaction.minimum.x + 3.0f, interaction.minimum.y + 3.0f),
            ImVec2(interaction.maximum.x - 3.0f, interaction.maximum.y - 3.0f),
            ImGui::GetColorU32(ToImVec4(palette.focus)), 2.0f, corners, 2.0f);
      }
    }

    const ImVec2 group_minimum = interactions.front().minimum;
    const ImVec2 group_maximum = interactions.back().maximum;
    draw_list->AddRect(group_minimum, group_maximum,
                       ImGui::GetColorU32(ToImVec4(palette.border_strong)),
                       kToolbarControlRounding, ImDrawFlags_RoundCornersAll);
    for (std::size_t index = 1; index < interactions.size(); ++index) {
      const float separator_x = interactions[index].minimum.x;
      draw_list->AddLine(ImVec2(separator_x, group_minimum.y),
                         ImVec2(separator_x, group_maximum.y),
                         ImGui::GetColorU32(ToImVec4(palette.border)));
    }
    ImGui::PopID();
  }

  void DrawToolbarPopover(const ApplicationView &view,
                          const ToolbarPopoverView &popover,
                          const bool icon_only) {
    if (!popover.availability.visible) {
      return;
    }
    if (ToolbarButton(popover.id.value, popover.label, popover.icon,
                      popover.tooltip, popover.availability, false,
                      icon_only)) {
      ImGui::OpenPopup(("##popup." + popover.id.value).c_str());
    }
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(224.0f, 0.0f),
        ImVec2(320.0f, std::numeric_limits<float>::max()));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,
                          ToImVec4(CurrentPalette().application_surface));
    if (ImGui::BeginPopup(("##popup." + popover.id.value).c_str())) {
      for (const ToolbarMenuItemView &item : popover.items) {
        if (item.separator_before) {
          ImGui::Separator();
        }
        const bool enabled =
            item.action.availability.enabled && !item.action.availability.busy;
        const char *secondary = item.secondary_label.empty()
                                    ? nullptr
                                    : item.secondary_label.c_str();
        if (ImGui::MenuItem(item.label.c_str(), secondary, item.selected,
                            enabled)) {
          EmitEdit(view.revision, item.action);
        }
        if (!enabled &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
            !item.action.availability.disabled_reason.empty()) {
          ImGui::SetTooltip("%s",
                            item.action.availability.disabled_reason.c_str());
        }
      }
      for (const FieldView &field : popover.fields) {
        ImGui::Separator();
        DrawField(view, field);
      }
      ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
  }

  void DrawToolbarItem(const ApplicationView &view, const ToolbarItemView &item,
                       const bool icon_only) {
    std::visit(
        [this, &view, icon_only](const auto &value) {
          using Item = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<Item, CommandView>) {
            if (ToolbarButton(value.id.value, value.label, value.icon,
                              value.tooltip, value.availability, false,
                              icon_only, value.variant)) {
              EmitCommand(view.revision, value);
            }
          } else if constexpr (std::is_same_v<Item, ToolbarSegmentedView>) {
            DrawToolbarSegmented(view, value);
          } else if constexpr (std::is_same_v<Item, ToolbarActionView>) {
            if (ToolbarButton(value.id.value, value.label, value.icon,
                              value.tooltip, value.action.availability,
                              value.selected, icon_only)) {
              EmitEdit(view.revision, value.action);
            }
          } else if constexpr (std::is_same_v<Item, ToolbarSeparatorView>) {
            const ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(1.0f, 24.0f));
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(cursor.x, cursor.y + 4.0f),
                ImVec2(cursor.x, cursor.y + 20.0f),
                ImGui::GetColorU32(ToImVec4(CurrentPalette().border)));
          } else if constexpr (std::is_same_v<Item, ToolbarSpacerView>) {
          } else if constexpr (std::is_same_v<Item, ToolbarPopoverView>) {
            DrawToolbarPopover(view, value, icon_only);
          }
        },
        item);
  }

  void DrawToolbarSequence(const ApplicationView &view,
                           const std::vector<ToolbarItemView> &items,
                           const std::size_t begin, const std::size_t end,
                           const bool icon_only, const bool horizontal) {
    for (std::size_t index = begin; index < end; ++index) {
      if (index > begin && horizontal) {
        ImGui::SameLine(0.0f, 8.0f);
      }
      DrawToolbarItem(view, items[index], icon_only);
    }
  }

  float ToolbarItemWidth(const ToolbarItemView &item,
                         const bool icon_only) const {
    return std::visit(
        [icon_only](const auto &value) {
          using Item = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<Item, CommandView>) {
            if (!value.availability.visible) {
              return 0.0f;
            }
            return icon_only
                       ? kToolbarControlHeight
                       : std::max(56.0f,
                                  ImGui::CalcTextSize(value.label.c_str()).x +
                                      24.0f);
          } else if constexpr (std::is_same_v<Item, ToolbarSegmentedView>) {
            float width = 0.0f;
            for (const ToolbarChoiceView &choice : value.choices) {
              if (choice.action.availability.visible) {
                width += std::max(
                    56.0f, ImGui::CalcTextSize(choice.label.c_str()).x + 24.0f);
              }
            }
            return width;
          } else if constexpr (std::is_same_v<Item, ToolbarActionView>) {
            if (!value.action.availability.visible) {
              return 0.0f;
            }
            return icon_only
                       ? kToolbarControlHeight
                       : std::max(56.0f,
                                  ImGui::CalcTextSize(value.label.c_str()).x +
                                      24.0f);
          } else if constexpr (std::is_same_v<Item, ToolbarSeparatorView>) {
            return 1.0f;
          } else if constexpr (std::is_same_v<Item, ToolbarSpacerView>) {
            return 0.0f;
          } else if constexpr (std::is_same_v<Item, ToolbarPopoverView>) {
            if (!value.availability.visible) {
              return 0.0f;
            }
            return icon_only
                       ? kToolbarControlHeight
                       : std::max(56.0f,
                                  ImGui::CalcTextSize(value.label.c_str()).x +
                                      24.0f);
          }
          return 0.0f;
        },
        item);
  }

  void DrawContextToolbar(const ApplicationView &view) {
    const std::vector<ToolbarItemView> &items = view.context_toolbar.items;
    if (items.empty()) {
      return;
    }
    const auto spacer = std::find_if(
        items.begin(), items.end(), [](const ToolbarItemView &item) {
          return std::holds_alternative<ToolbarSpacerView>(item);
        });
    const std::size_t split =
        static_cast<std::size_t>(std::distance(items.begin(), spacer));
    ImGui::SetCursorPos(ImVec2(12.0f, 4.0f));
    if (spacer == items.end()) {
      DrawToolbarSequence(view, items, 0, items.size(), false, true);
      return;
    }
    DrawToolbarSequence(view, items, 0, split, false, true);
    float right_width = 0.0f;
    std::size_t right_count = 0;
    for (std::size_t index = split + 1; index < items.size(); ++index) {
      const float item_width = ToolbarItemWidth(items[index], false);
      if (item_width > 0.0f) {
        right_width += item_width;
        ++right_count;
      }
    }
    if (right_count > 1) {
      right_width += static_cast<float>(right_count - 1) * 8.0f;
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(std::max(
        ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - right_width - 12.0f));
    DrawToolbarSequence(view, items, split + 1, items.size(), false, true);
  }

  void DrawActivityRail(const ApplicationView &view) {
    for (const ActivityView &activity : view.activities) {
      const NavigationItemResult result = NavigationItem({
          .id = "activity." +
                std::to_string(static_cast<int>(activity.destination)),
          .label = activity.label,
          .tooltip = activity.availability.disabled_reason,
          .selected = activity.destination == session.active_destination,
          .availability = ToAvailability(activity.availability),
      });
      if (result.activated &&
          activity.destination != session.active_destination) {
        session.ActivateDestination(activity.destination);
        navigation_changed = true;
      }
    }
  }

  void DrawExplorer(const ApplicationView &view) {
    if (bold_font != nullptr) {
      ImGui::PushFont(bold_font);
    }
    ImGui::TextUnformatted(view.explorer.title.c_str());
    if (bold_font != nullptr) {
      ImGui::PopFont();
    }

    std::string &query = session.explorer_queries[session.active_destination];
    std::array<char, 256> buffer{};
    const std::size_t copy_length =
        std::min(query.size(), buffer.size() - std::size_t{1});
    std::copy_n(query.data(), copy_length, buffer.data());
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputTextWithHint("##explorer-filter",
                                 view.explorer.search_placeholder.c_str(),
                                 buffer.data(), buffer.size())) {
      query = buffer.data();
    }

    for (const CommandView &command : view.explorer.commands) {
      DrawCommandButton(view.revision, command, true);
      ImGui::SameLine();
    }
    if (!view.explorer.commands.empty()) {
      ImGui::NewLine();
    }

    for (const TreeRowView &row : view.explorer.rows) {
      if (!row.visible || !MatchesQuery(row, query)) {
        continue;
      }
      ImGui::Indent(static_cast<float>(row.depth) * 14.0f);
      std::string label = row.label;
      if (!row.secondary_label.empty()) {
        label += "  " + row.secondary_label;
      }
      if (ImGui::Selectable((label + "##" + row.id.value).c_str(), row.selected,
                            ImGuiSelectableFlags_AllowDoubleClick)) {
        const ImGuiIO &io = ImGui::GetIO();
        intents.emplace_back(ChangeSelection{
            .revision = view.revision,
            .entity = row.id,
            .additive = io.KeyCtrl,
            .range = io.KeyShift,
        });
      }
      ImGui::Unindent(static_cast<float>(row.depth) * 14.0f);
    }
  }

  PointerState CapturePointer(const ImVec2 minimum,
                              const ImVec2 framebuffer_scale) {
    const ImGuiIO &io = ImGui::GetIO();
    const bool hovered =
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    return {
        .position = {(io.MousePos.x - minimum.x) * framebuffer_scale.x,
                     (io.MousePos.y - minimum.y) * framebuffer_scale.y},
        .delta = {io.MouseDelta.x * framebuffer_scale.x,
                  io.MouseDelta.y * framebuffer_scale.y},
        .wheel = io.MouseWheel,
        .hovered = hovered || ImGui::IsItemActive(),
        .primary_down = ImGui::IsMouseDown(ImGuiMouseButton_Left),
        .primary_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left),
        .primary_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left),
        .secondary_down = ImGui::IsMouseDown(ImGuiMouseButton_Right),
        .secondary_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right),
        .secondary_released = ImGui::IsMouseReleased(ImGuiMouseButton_Right),
        .shift = io.KeyShift,
        .control = io.KeyCtrl,
        .alt = io.KeyAlt,
    };
  }

  void DrawModelSurface(const ApplicationView &view,
                        const SurfaceBindings &surfaces) {
    if (surfaces.model == nullptr) {
      ImGui::TextDisabled("%s", view.workspace.empty_message.c_str());
      return;
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImGuiIO &io = ImGui::GetIO();
    const Vec2 framebuffer_scale{
        std::max(io.DisplayFramebufferScale.x, 1.0f),
        std::max(io.DisplayFramebufferScale.y, 1.0f),
    };
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("##model-surface", available,
                           ImGuiButtonFlags_MouseButtonLeft |
                               ImGuiButtonFlags_MouseButtonRight);
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    PointerState pointer = CapturePointer(
        minimum, ImVec2(framebuffer_scale.x, framebuffer_scale.y));
    if (!view.workspace.viewport_toolbar.empty()) {
      const ImVec2 mouse = ImGui::GetIO().MousePos;
      const float toolbar_height =
          static_cast<float>(view.workspace.viewport_toolbar.size()) * 32.0f +
          static_cast<float>(view.workspace.viewport_toolbar.size() - 1) * 4.0f;
      if (mouse.x >= minimum.x + 16.0f && mouse.x <= minimum.x + 48.0f &&
          mouse.y >= minimum.y + 16.0f &&
          mouse.y <= minimum.y + 16.0f + toolbar_height) {
        pointer.hovered = false;
        pointer.primary_clicked = false;
        pointer.primary_released = false;
      }
    }
    const ModelSurfaceFrame frame = surfaces.model->Render({
        .logical_size = {available.x, available.y},
        .framebuffer_scale = framebuffer_scale,
        .selection_tool = view.workspace.model_selection_tool,
        .pointer = pointer,
    });
    if (frame.ready && frame.texture) {
      ImGui::GetWindowDrawList()->AddImage(ToImTextureId(frame.texture),
                                           minimum, maximum, ImVec2(0.0f, 1.0f),
                                           ImVec2(1.0f, 0.0f));
    } else {
      ImGui::GetWindowDrawList()->AddText(
          minimum, ImGui::GetColorU32(ImGuiCol_TextDisabled),
          view.workspace.empty_message.c_str());
    }
    if (frame.selection_marquee.has_value()) {
      const Rect &marquee = *frame.selection_marquee;
      const ImVec2 marquee_minimum(
          minimum.x + std::min(marquee.minimum.x, marquee.maximum.x),
          minimum.y + std::min(marquee.minimum.y, marquee.maximum.y));
      const ImVec2 marquee_maximum(
          minimum.x + std::max(marquee.minimum.x, marquee.maximum.x),
          minimum.y + std::max(marquee.minimum.y, marquee.maximum.y));
      const SemanticPalette &palette = CurrentPalette();
      ColorRgba fill = palette.selection;
      fill.alpha = 0.18f;
      ImGui::GetWindowDrawList()->AddRectFilled(
          marquee_minimum, marquee_maximum, ImGui::GetColorU32(ToImVec4(fill)));
      ImGui::GetWindowDrawList()->AddRect(
          marquee_minimum, marquee_maximum,
          ImGui::GetColorU32(ToImVec4(palette.focus)), 0.0f, 0, 1.0f);
    }
  }

  void DrawCanvasSurface(const ApplicationView &view,
                         const SurfaceBindings &surfaces) {
    if (surfaces.canvas == nullptr || !surfaces.canvas->valid()) {
      ImGui::TextDisabled("%s", view.workspace.empty_message.c_str());
      return;
    }
    im2d::DrawCanvas(*surfaces.canvas->state_);
  }

  void DrawWorkspace(const ApplicationView &view,
                     const SurfaceBindings &surfaces) {
    if (view.workspace.kind == WorkspaceKind::Model3d) {
      DrawModelSurface(view, surfaces);
      if (!view.workspace.viewport_toolbar.empty()) {
        const ImVec2 surface_minimum = ImGui::GetItemRectMin();
        ImGui::SetCursorScreenPos(
            ImVec2(surface_minimum.x + 16.0f, surface_minimum.y + 16.0f));
        ImGui::BeginGroup();
        DrawToolbarSequence(view, view.workspace.viewport_toolbar, 0,
                            view.workspace.viewport_toolbar.size(), true,
                            false);
        ImGui::EndGroup();
      }
    } else {
      DrawCanvasSurface(view, surfaces);
    }
  }

  void DrawField(const ApplicationView &view, const FieldView &field) {
    if (!field.availability.visible) {
      return;
    }
    ImGui::PushID(field.id.value.c_str());
    ImGui::BeginDisabled(!field.availability.enabled ||
                         field.availability.busy);
    if (const bool *value = std::get_if<bool>(&field.value)) {
      bool edited = *value;
      if (ImGui::Checkbox(field.label.c_str(), &edited)) {
        intents.emplace_back(EditField{
            .revision = view.revision,
            .field = field.id,
            .value = edited,
            .target = field.target,
            .phase = EditPhase::Commit,
        });
      }
    } else if (const std::int64_t *value =
                   std::get_if<std::int64_t>(&field.value)) {
      std::int64_t edited = *value;
      if (ImGui::InputScalar(field.label.c_str(), ImGuiDataType_S64, &edited)) {
        intents.emplace_back(EditField{view.revision, field.id, edited,
                                       field.target, EditPhase::Changed});
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        intents.emplace_back(EditField{view.revision, field.id, edited,
                                       field.target, EditPhase::Commit});
      }
    } else if (const double *value = std::get_if<double>(&field.value)) {
      double edited = *value;
      if (ImGui::InputDouble(field.label.c_str(), &edited, 0.0, 0.0, "%.3f")) {
        intents.emplace_back(EditField{view.revision, field.id, edited,
                                       field.target, EditPhase::Changed});
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        intents.emplace_back(EditField{view.revision, field.id, edited,
                                       field.target, EditPhase::Commit});
      }
    } else if (const std::string *value =
                   std::get_if<std::string>(&field.value)) {
      std::array<char, 512> buffer{};
      const std::size_t length =
          std::min(value->size(), buffer.size() - std::size_t{1});
      std::copy_n(value->data(), length, buffer.data());
      if (ImGui::InputText(field.label.c_str(), buffer.data(), buffer.size())) {
        intents.emplace_back(EditField{view.revision, field.id,
                                       std::string(buffer.data()), field.target,
                                       EditPhase::Changed});
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        intents.emplace_back(EditField{view.revision, field.id,
                                       std::string(buffer.data()), field.target,
                                       EditPhase::Commit});
      }
    }
    ImGui::EndDisabled();
    if (!field.help.empty()) {
      ImGui::TextDisabled("%s", field.help.c_str());
    }
    if (!field.availability.enabled &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
        !field.availability.disabled_reason.empty()) {
      ImGui::SetTooltip("%s", field.availability.disabled_reason.c_str());
    }
    ImGui::PopID();
  }

  void DrawInspector(const ApplicationView &view) {
    if (bold_font != nullptr) {
      ImGui::PushFont(bold_font);
    }
    ImGui::TextUnformatted(view.inspector.title.c_str());
    if (bold_font != nullptr) {
      ImGui::PopFont();
    }
    if (view.inspector.sections.empty()) {
      ImGui::TextWrapped("%s", view.inspector.empty_message.c_str());
    }
    for (const SectionView &section : view.inspector.sections) {
      ImGui::PushID(section.id.value.c_str());
      ImGui::SetNextItemOpen(section.initially_open, ImGuiCond_Once);
      if (ImGui::CollapsingHeader(section.heading.c_str())) {
        for (const FieldView &field : section.fields) {
          DrawField(view, field);
        }
        for (const CommandView &command : section.commands) {
          DrawCommandButton(view.revision, command, false);
        }
      }
      ImGui::PopID();
    }
    for (const CommandView &command : view.inspector.primary_commands) {
      DrawCommandButton(view.revision, command, false);
    }
  }

  void DrawOperationTray(const ApplicationView &view) {
    if (!view.operation.has_value()) {
      return;
    }
    ImGui::TextWrapped("%s", view.operation->summary.c_str());
  }

  void DrawOperationStrip(const ApplicationView &view) {
    if (!view.operation.has_value()) {
      return;
    }
    const OperationView &operation = *view.operation;
    StatusText({.label = operation.title, .status = ToStatus(operation.tone)});
    if (!operation.indeterminate) {
      ImGui::SameLine();
      ImGui::ProgressBar(std::clamp(operation.progress, 0.0f, 1.0f),
                         ImVec2(180.0f, 0.0f));
    }
    for (const CommandView &command : operation.commands) {
      ImGui::SameLine();
      DrawCommandButton(view.revision, command, true);
    }
  }

  void DrawStatusBar(const ApplicationView &view) {
    bool first = true;
    for (const StatusItemView &item : view.status_items) {
      if (!first) {
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
      }
      StatusText({.label = item.label, .status = ToStatus(item.tone)});
      first = false;
    }
  }

  const CommandView *FindCommand(const ApplicationView &view,
                                 const CommandId id) const {
    if (const CommandView *menu_command =
            FindMenuCommand(view.application_bar, id);
        menu_command != nullptr) {
      return menu_command;
    }
    const std::array<const std::vector<ToolbarItemView> *, 2> groups = {
        &view.context_toolbar.items,
        &view.workspace.viewport_toolbar,
    };
    for (const std::vector<ToolbarItemView> *group : groups) {
      for (const ToolbarItemView &item : *group) {
        if (const CommandView *command = std::get_if<CommandView>(&item);
            command != nullptr && command->command == id) {
          return command;
        }
      }
    }
    return nullptr;
  }

  void HandleShortcuts(const ApplicationView &view) {
    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantTextInput) {
      return;
    }
    const auto invoke = [this, &view](const CommandId id) {
      if (const CommandView *command = FindCommand(view, id);
          command != nullptr) {
        EmitCommand(view.revision, *command);
      }
    };
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
      invoke(CommandId::OpenFile);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
      invoke(CommandId::Quit);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false)) {
      invoke(CommandId::SelectAll);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
      invoke(CommandId::ClearSelection);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F, false) && !io.KeyCtrl) {
      invoke(io.KeyShift ? CommandId::ZoomToSelection : CommandId::ZoomToFit);
    }
  }
};

ApplicationUi::ApplicationUi() : impl_(std::make_unique<Impl>()) {}
ApplicationUi::~ApplicationUi() = default;
ApplicationUi::ApplicationUi(ApplicationUi &&) noexcept = default;
ApplicationUi &ApplicationUi::operator=(ApplicationUi &&) noexcept = default;

AssetLoadReport
ApplicationUi::Initialize(const std::filesystem::path &asset_root,
                          const float dpi_scale) {
  AssetLoadReport report;
  if (ImGui::GetCurrentContext() == nullptr) {
    report.used_fallback_font = true;
    report.messages.emplace_back(
        "Fancy UI initialization requires an active ImGui context");
    return report;
  }

  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->Clear();
  const float scale = std::clamp(dpi_scale, 0.75f, 2.0f);
  impl_->application_menu_font_size = Impl::kApplicationMenuFontSize * scale;
  const auto load = [&report, &asset_root](const char *name,
                                           const float size) -> ImFont * {
    const std::filesystem::path path = asset_root / "fonts" / name;
    if (!std::filesystem::is_regular_file(path)) {
      report.messages.push_back("Missing UI font: " + path.string());
      return nullptr;
    }
    ImFont *font =
        ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str(), size);
    if (font == nullptr) {
      report.messages.push_back("Could not load UI font: " + path.string());
    }
    return font;
  };

  impl_->regular_font = load("NotoSans-Regular.ttf", 16.0f * scale);
  impl_->bold_font = load("NotoSans-Bold.ttf", 18.0f * scale);
  impl_->mono_font = load("NotoSansMono-Regular.ttf", 16.0f * scale);
  constexpr std::array<const char *, 14> kRequiredIcons = {
      "alert.svg",   "arrow-right.svg", "camera.svg",  "check.svg",
      "compact.svg", "eye.svg",         "focus.svg",   "gear.svg",
      "grain.svg",   "lock.svg",        "package.svg", "plus.svg",
      "search.svg",  "unlock.svg",
  };
  for (const char *icon : kRequiredIcons) {
    const std::filesystem::path path = asset_root / "icons" / icon;
    if (!std::filesystem::is_regular_file(path)) {
      report.messages.push_back("Missing UI icon: " + path.string());
    }
  }
  if (impl_->regular_font == nullptr) {
    impl_->regular_font = io.Fonts->AddFontDefault();
    report.used_fallback_font = true;
  }
  const std::array icon_sources{
      std::pair{"fit", "package.svg"},
      std::pair{"focus", "focus.svg"},
      std::pair{"view", "camera.svg"},
      std::pair{"visibility", "eye.svg"},
      std::pair{"orbit-locked", "lock.svg"},
      std::pair{"orbit-unlocked", "unlock.svg"},
      std::pair{"send", "arrow-right.svg"},
  };
  impl_->icon_rects.clear();
  impl_->pending_icons.clear();
  const int icon_pixels =
      std::max(16, static_cast<int>(std::round(16.0f * scale)));
  for (const auto &[name, filename] : icon_sources) {
    const std::filesystem::path path = asset_root / "icons" / filename;
    std::unique_ptr<lunasvg::Document> document =
        lunasvg::Document::loadFromFile(path.string());
    if (document == nullptr) {
      continue;
    }
    lunasvg::Bitmap bitmap = document->renderToBitmap(icon_pixels, icon_pixels);
    if (bitmap.isNull()) {
      report.messages.push_back("Could not rasterize UI icon: " +
                                path.string());
      continue;
    }
    bitmap.convertToRGBA();
    impl_->pending_icons.push_back({.name = name, .bitmap = std::move(bitmap)});
  }
  io.FontDefault = impl_->regular_font;
  ApplyTheme(ResolvedTheme::Dark);
  return report;
}

const SessionState &ApplicationUi::session() const { return impl_->session; }

void ApplicationUi::SetSession(SessionState session) {
  session.ActivateDestination(session.active_destination);
  impl_->session = std::move(session);
}

FrameResult ApplicationUi::Draw(const ApplicationView &view,
                                const SurfaceBindings &surfaces) {
  impl_->intents.clear();
  impl_->navigation_changed = false;
  impl_->layout_changed = false;
  impl_->InstallPendingIcons();
  if (impl_->session.active_destination != view.active_destination) {
    impl_->session.ActivateDestination(view.active_destination);
  }

  ApplyTheme(ResolveTheme(view.theme_mode));
  impl_->DrawApplicationBar(view);
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("##steppenface-application", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  shell::ApplicationShellState shell_state{
      .explorer_visible = impl_->session.explorer_visible,
      .inspector_visible = impl_->session.inspector_visible,
      .operation_tray_visible = impl_->session.operation_tray_visible,
      .explorer_width = impl_->session.explorer_width,
      .inspector_width = impl_->session.inspector_width,
      .operation_tray_height = impl_->session.operation_tray_height,
  };
  const shell::ApplicationShellSpec shell_spec{
      .application_bar = {.id = "application-bar",
                          .draw = {},
                          .visible = false},
      .context_toolbar = {.id = "context-toolbar",
                          .draw =
                              [this, &view]() {
                                impl_->DrawContextToolbar(view);
                              }},
      .activity_rail = {.id = "activity-rail",
                        .draw = [this,
                                 &view]() { impl_->DrawActivityRail(view); }},
      .explorer = {.id = "explorer",
                   .draw = [this, &view]() { impl_->DrawExplorer(view); }},
      .workspace = {.id = "workspace",
                    .draw =
                        [this, &view, &surfaces]() {
                          impl_->DrawWorkspace(view, surfaces);
                        }},
      .inspector = {.id = "inspector",
                    .draw = [this, &view]() { impl_->DrawInspector(view); }},
      .operation_tray = {.id = "operation-tray",
                         .draw = [this,
                                  &view]() { impl_->DrawOperationTray(view); },
                         .visible = view.operation.has_value()},
      .operation_strip = {.id = "operation-strip",
                          .draw =
                              [this, &view]() {
                                impl_->DrawOperationStrip(view);
                              },
                          .visible = view.operation.has_value()},
      .status_bar = {.id = "status-bar",
                     .draw = [this, &view]() { impl_->DrawStatusBar(view); }},
  };
  const shell::ApplicationShellResult shell_result =
      shell::Application(shell_spec, shell_state);
  impl_->session.explorer_width = shell_result.state.explorer_width;
  impl_->session.inspector_width = shell_result.state.inspector_width;
  impl_->session.operation_tray_height =
      shell_result.state.operation_tray_height;
  impl_->layout_changed = shell_result.layout_changed;
  impl_->HandleShortcuts(view);

  ImGui::End();
  ImGui::PopStyleVar();
  return {
      .product_intents = std::move(impl_->intents),
      .backend_requests = {},
      .navigation_changed = impl_->navigation_changed,
      .layout_changed = impl_->layout_changed,
  };
}

} // namespace fancy_ui::steppenface
