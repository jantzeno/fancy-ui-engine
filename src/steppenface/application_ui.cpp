#include "fancy_ui/steppenface/application_ui.hpp"

#include "fancy_ui/components/button.hpp"
#include "fancy_ui/components/checkbox.hpp"
#include "fancy_ui/components/data_display.hpp"
#include "fancy_ui/components/feedback.hpp"
#include "fancy_ui/components/hierarchy.hpp"
#include "fancy_ui/components/navigation.hpp"
#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/shell/application.hpp"
#include "fancy_ui/steppenface/ui_assets.hpp"
#include "fancy_ui/theme.hpp"

#include "internal/application_chrome.hpp"
#include "internal/component_internal.hpp"
#include "internal/operation_disclosure.hpp"
#include "internal/ui_asset_atlas.hpp"
#include "ui/im2d_canvas_widget.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
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

std::size_t ExplorerSubtreeEnd(const std::vector<TreeRowView> &rows,
                               const std::size_t index) {
  std::size_t end = index + 1;
  while (end < rows.size() && rows[end].depth > rows[index].depth) {
    ++end;
  }
  return end;
}

bool ExplorerSubtreeMatches(const std::vector<TreeRowView> &rows,
                            const std::size_t index, const std::string &query) {
  if (!rows[index].visible) {
    return false;
  }
  const std::size_t end = ExplorerSubtreeEnd(rows, index);
  for (std::size_t descendant = index; descendant < end; ++descendant) {
    if (rows[descendant].visible && MatchesQuery(rows[descendant], query)) {
      return true;
    }
  }
  return false;
}

FontHandle NativeFontHandle(ImFont *font) {
  return {.value = reinterpret_cast<std::uintptr_t>(font)};
}

} // namespace

class ApplicationUi::Impl {
public:
  static constexpr float kToolbarControlRounding = 4.0f;
  static constexpr float kToolbarLabelOffsetY = -1.0f;

  struct ToolbarSegmentInteraction {
    const ToolbarChoiceView *choice = nullptr;
    ImVec2 minimum;
    ImVec2 maximum;
    bool hovered = false;
    bool pressed = false;
    bool keyboard_focused = false;
    bool disabled = false;
  };

  SessionState session;
  std::filesystem::path asset_root;
  detail::UiAssetAtlas assets;
  detail::ApplicationChrome chrome{assets};
  std::vector<UiIntent> intents;
  bool navigation_changed = false;
  bool layout_changed = false;

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

  void ToggleLayout(const detail::LayoutRegion region) {
    switch (region) {
    case detail::LayoutRegion::Explorer:
      session.explorer_visible = !session.explorer_visible;
      break;
    case detail::LayoutRegion::OperationTray:
      session.operation_tray_visible = !session.operation_tray_visible;
      break;
    case detail::LayoutRegion::Inspector:
      session.inspector_visible = !session.inspector_visible;
      break;
    }
    layout_changed = true;
  }

  [[nodiscard]] detail::ChromeLayoutState
  ChromeLayout(const bool operation_available) const {
    return {
        .explorer_visible = session.explorer_visible,
        .operation_tray_visible = session.operation_tray_visible,
        .operation_available = operation_available,
        .inspector_visible = session.inspector_visible,
    };
  }

  [[nodiscard]] detail::ApplicationChromeCallbacks
  ChromeCallbacks(const ApplicationView &view) {
    return {
        .invoke_command =
            [this, revision = view.revision](const CommandView &command) {
              EmitCommand(revision, command);
            },
        .commit_action =
            [this, revision = view.revision](const ControlActionView &action) {
              EmitEdit(revision, action);
            },
        .draw_field =
            [this, &view](const FieldView &field) { DrawField(view, field); },
        .activate_workspace =
            [this](const WorkspaceKind workspace) {
              ActivateWorkspace(workspace);
            },
        .toggle_layout =
            [this](const detail::LayoutRegion region) { ToggleLayout(region); },
    };
  }

  bool DrawAtlasIcon(const std::string_view icon, const UiIconSize size,
                     const ImVec2 minimum, const ImVec2 maximum,
                     const ImU32 color) {
    const ImVec4 tint = ImGui::ColorConvertU32ToFloat4(color);
    return assets.DrawIcon(
        icon, size,
        {.minimum = {.x = minimum.x, .y = minimum.y},
         .maximum = {.x = maximum.x, .y = maximum.y}},
        {.red = tint.x, .green = tint.y, .blue = tint.z, .alpha = tint.w});
  }

  void DrawToolbarIcon(const std::string &icon, const ImVec2 minimum,
                       const ImVec2 maximum, const ImU32 color) {
    const ImVec2 center((minimum.x + maximum.x) * 0.5f,
                        (minimum.y + maximum.y) * 0.5f);
    const float half_icon = CurrentLayoutMetrics().geometry.icon * 0.5f;
    static_cast<void>(DrawAtlasIcon(
        icon, UiIconSize::Small16,
        ImVec2(center.x - half_icon, center.y - half_icon),
        ImVec2(center.x + half_icon, center.y + half_icon), color));
  }

  bool ToolbarButton(const std::string &id, const std::string &label,
                     const std::string &icon, const std::string &tooltip,
                     const Availability &availability, const bool selected,
                     const bool icon_only, const bool has_popup,
                     const CommandVariant variant = CommandVariant::Normal) {
    if (!availability.visible) {
      return false;
    }
    const LayoutMetrics metrics = CurrentLayoutMetrics();
    const SemanticPalette &palette = CurrentPalette();
    const bool disabled = !availability.enabled || availability.busy;
    const float disclosure_width =
        !icon_only && has_popup ? metrics.geometry.icon : 0.0f;
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
                             Scale(kToolbarControlRounding));
    draw_list->AddRect(minimum, maximum, ImGui::GetColorU32(ToImVec4(border)),
                       Scale(kToolbarControlRounding));
    if (icon_only) {
      DrawToolbarIcon(icon, minimum, maximum,
                      ImGui::GetColorU32(ToImVec4(foreground)));
    } else {
      const ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
      const float label_maximum_x = maximum.x - disclosure_width;
      draw_list->AddText(
          ImVec2(
              std::floor((minimum.x + label_maximum_x - label_size.x) * 0.5f),
              std::floor((minimum.y + maximum.y - label_size.y) * 0.5f +
                         Scale(kToolbarLabelOffsetY))),
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
    if (keyboard_focused) {
      draw_list->AddRect(
          ImVec2(minimum.x + Scale(3.0f), minimum.y + Scale(3.0f)),
          ImVec2(maximum.x - Scale(3.0f), maximum.y - Scale(3.0f)),
          ImGui::GetColorU32(ToImVec4(palette.focus)), Scale(2.0f),
          ImDrawFlags_RoundCornersAll, Scale(2.0f));
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      const std::string &message =
          availability.enabled ? tooltip : availability.disabled_reason;
      if (!message.empty()) {
        detail::ShowTooltip(message);
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

    const LayoutMetrics metrics = CurrentLayoutMetrics();
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
          detail::ShowTooltip(message);
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
                               Scale(kToolbarControlRounding), corners);

      const ImVec2 label_size =
          ImGui::CalcTextSize(interaction.choice->label.c_str());
      draw_list->AddText(
          ImVec2(std::floor((interaction.minimum.x + interaction.maximum.x -
                             label_size.x) *
                            0.5f),
                 std::floor((interaction.minimum.y + interaction.maximum.y -
                             label_size.y) *
                                0.5f +
                            Scale(kToolbarLabelOffsetY))),
          ImGui::GetColorU32(ToImVec4(foreground)),
          interaction.choice->label.c_str());
      if (selected) {
        draw_list->AddRectFilled(ImVec2(interaction.minimum.x + Scale(1.0f),
                                        interaction.maximum.y - Scale(3.0f)),
                                 ImVec2(interaction.maximum.x - Scale(1.0f),
                                        interaction.maximum.y - Scale(1.0f)),
                                 ImGui::GetColorU32(ToImVec4(palette.focus)));
      }
      if (interaction.keyboard_focused) {
        draw_list->AddRect(ImVec2(interaction.minimum.x + Scale(3.0f),
                                  interaction.minimum.y + Scale(3.0f)),
                           ImVec2(interaction.maximum.x - Scale(3.0f),
                                  interaction.maximum.y - Scale(3.0f)),
                           ImGui::GetColorU32(ToImVec4(palette.focus)),
                           Scale(2.0f), corners, Scale(2.0f));
      }
    }

    const ImVec2 group_minimum = interactions.front().minimum;
    const ImVec2 group_maximum = interactions.back().maximum;
    draw_list->AddRect(group_minimum, group_maximum,
                       ImGui::GetColorU32(ToImVec4(palette.border_strong)),
                       Scale(kToolbarControlRounding),
                       ImDrawFlags_RoundCornersAll);
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
                      popover.tooltip, popover.availability, false, icon_only,
                      true)) {
      ImGui::OpenPopup(("##popup." + popover.id.value).c_str());
    }
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(Scale(224.0f), 0.0f),
        ImVec2(Scale(320.0f), std::numeric_limits<float>::max()));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(Scale(8.0f), Scale(8.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(Scale(8.0f), Scale(4.0f)));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,
                          ToImVec4(CurrentPalette().application_surface));
    if (ImGui::BeginPopup(("##popup." + popover.id.value).c_str())) {
      for (const ToolbarPopoverItemView &item : popover.items) {
        std::visit(
            [this, &view](const auto &value) {
              using Item = std::decay_t<decltype(value)>;
              if constexpr (std::is_same_v<Item, CommandView>) {
                if (!value.availability.visible) {
                  return;
                }
                ImGui::PushID(value.id.value.c_str());
                const bool enabled =
                    value.availability.enabled && !value.availability.busy;
                const char *shortcut =
                    value.shortcut.empty() ? nullptr : value.shortcut.c_str();
                if (ImGui::MenuItem(value.label.c_str(), shortcut, false,
                                    enabled)) {
                  EmitCommand(view.revision, value);
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                  const std::string &message =
                      enabled ? value.tooltip
                              : value.availability.disabled_reason;
                  if (!message.empty()) {
                    detail::ShowTooltip(message);
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
                const bool enabled = value.action.availability.enabled &&
                                     !value.action.availability.busy;
                const char *secondary = value.secondary_label.empty()
                                            ? nullptr
                                            : value.secondary_label.c_str();
                if (ImGui::MenuItem(value.label.c_str(), secondary,
                                    value.selected, enabled)) {
                  EmitEdit(view.revision, value.action);
                }
                if (!enabled &&
                    ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
                    !value.action.availability.disabled_reason.empty()) {
                  detail::ShowTooltip(
                      value.action.availability.disabled_reason);
                }
                ImGui::PopID();
              }
            },
            item);
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
                              icon_only, false, value.variant)) {
              EmitCommand(view.revision, value);
            }
          } else if constexpr (std::is_same_v<Item, ToolbarSegmentedView>) {
            DrawToolbarSegmented(view, value);
          } else if constexpr (std::is_same_v<Item, ToolbarActionView>) {
            if (ToolbarButton(value.id.value, value.label, value.icon,
                              value.tooltip, value.action.availability,
                              value.selected, icon_only, false)) {
              EmitEdit(view.revision, value.action);
            }
          } else if constexpr (std::is_same_v<Item, ToolbarSeparatorView>) {
            const LayoutMetrics metrics = CurrentLayoutMetrics();
            const ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(metrics.geometry.border,
                                metrics.geometry.compact_target));
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(cursor.x, cursor.y + metrics.spacing.space02),
                ImVec2(cursor.x, cursor.y + metrics.geometry.compact_target -
                                     metrics.spacing.space02),
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
    const float horizontal_spacing = CurrentLayoutMetrics().spacing.space03;
    for (std::size_t index = begin; index < end; ++index) {
      if (index > begin && horizontal) {
        ImGui::SameLine(0.0f, horizontal_spacing);
      }
      DrawToolbarItem(view, items[index], icon_only);
    }
  }

  void DrawActivityRail(const ApplicationView &view) {
    const WorkspaceKind active_workspace =
        WorkspaceForDestination(session.active_destination);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    for (const ActivityView &activity : view.activities) {
      if (!activity.availability.visible ||
          WorkspaceForDestination(activity.destination) != active_workspace) {
        continue;
      }
      const NavigationItemResult result = NavigationItem({
          .id = "activity." +
                std::to_string(static_cast<int>(activity.destination)),
          .label = activity.label,
          .tooltip = activity.label,
          .selected = activity.destination == session.active_destination,
          .availability = ToAvailability(activity.availability),
          .draw_icon =
              [this, &activity](const Rect &bounds, const ColorRgba color) {
                static_cast<void>(
                    DrawAtlasIcon(activity.icon, UiIconSize::Rail24,
                                  ImVec2(bounds.minimum.x, bounds.minimum.y),
                                  ImVec2(bounds.maximum.x, bounds.maximum.y),
                                  ImGui::GetColorU32(ToImVec4(color))));
              },
      });
      if (result.activated &&
          activity.destination != session.active_destination) {
        session.ActivateDestination(activity.destination);
        navigation_changed = true;
      }
    }
    ImGui::PopStyleVar();
  }

  void DrawExplorerNode(const ApplicationView &view, const std::string &query,
                        const std::size_t index, HierarchyTree &tree) {
    const std::vector<TreeRowView> &rows = view.explorer.rows;
    if (!ExplorerSubtreeMatches(rows, index, query)) {
      return;
    }

    const TreeRowView &row = rows[index];
    const std::size_t subtree_end = ExplorerSubtreeEnd(rows, index);
    const bool expandable = row.expandable || subtree_end > index + 1;
    auto [expansion, inserted] =
        session.explorer_expanded_rows.try_emplace(row.id.value, row.expanded);
    static_cast<void>(inserted);
    const bool expanded =
        expandable && (query.empty() ? expansion->second : true);
    std::string tooltip = row.label;
    if (!row.secondary_label.empty()) {
      tooltip += " · ";
      tooltip += row.secondary_label;
    }
    const HierarchyRowResult result = HierarchyRow(
        tree, {
                  .id = row.id.value,
                  .label = row.label,
                  .metadata = row.secondary_label,
                  .tooltip = tooltip,
                  .expandable = expandable,
                  .expanded = expanded,
                  .selected = row.selected,
                  .leading_icon = row.icon.empty() ? IconPainter{}
                                                   : assets.Painter(row.icon),
              });
    if (result.expansion_changed && query.empty()) {
      expansion->second = result.expanded;
    }
    if (result.activated) {
      intents.emplace_back(ChangeSelection{
          .revision = view.revision,
          .entity = row.id,
          .additive = result.additive,
          .range = result.range,
      });
    }

    if (!expandable || !result.expanded) {
      return;
    }
    std::size_t child = index + 1;
    while (child < subtree_end) {
      DrawExplorerNode(view, query, child, tree);
      child = ExplorerSubtreeEnd(rows, child);
    }
    tree.Pop();
  }

  void DrawExplorer(const ApplicationView &view) {
    if (assets.heading_font() != nullptr) {
      ImGui::PushFont(
          assets.heading_font(),
          CurrentLayoutMetrics().typography.section_heading_font_height);
    }
    ImGui::TextUnformatted(view.explorer.title.c_str());
    if (assets.heading_font() != nullptr) {
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

    HierarchyTree tree(
        {.section_font = NativeFontHandle(assets.heading_font())});
    std::size_t root = 0;
    while (root < view.explorer.rows.size()) {
      DrawExplorerNode(view, query, root, tree);
      root = ExplorerSubtreeEnd(view.explorer.rows, root);
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
      detail::DrawSecondaryText(view.workspace.empty_message);
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
      const LayoutMetrics metrics = CurrentLayoutMetrics();
      const ImVec2 mouse = ImGui::GetIO().MousePos;
      const float toolbar_height =
          static_cast<float>(view.workspace.viewport_toolbar.size()) *
              metrics.geometry.control_height +
          static_cast<float>(view.workspace.viewport_toolbar.size() - 1) *
              metrics.spacing.space03;
      if (mouse.x >= minimum.x + metrics.spacing.space05 &&
          mouse.x <= minimum.x + metrics.spacing.space05 +
                         metrics.geometry.control_height &&
          mouse.y >= minimum.y + metrics.spacing.space05 &&
          mouse.y <= minimum.y + metrics.spacing.space05 + toolbar_height) {
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
          minimum,
          ImGui::GetColorU32(ToImVec4(CurrentPalette().text_secondary)),
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
      detail::DrawSecondaryText(view.workspace.empty_message);
      return;
    }
    im2d::DrawCanvas(*surfaces.canvas->state_);
  }

  void DrawWorkspace(const ApplicationView &view,
                     const SurfaceBindings &surfaces) {
    if (view.workspace.kind == WorkspaceKind::Model3d) {
      DrawModelSurface(view, surfaces);
      if (!view.workspace.viewport_toolbar.empty()) {
        const LayoutMetrics metrics = CurrentLayoutMetrics();
        const ImVec2 surface_minimum = ImGui::GetItemRectMin();
        ImGui::SetCursorScreenPos(
            ImVec2(surface_minimum.x + metrics.spacing.space05,
                   surface_minimum.y + metrics.spacing.space05));
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
      detail::DrawSecondaryText(field.help);
    }
    if (!field.availability.enabled &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
        !field.availability.disabled_reason.empty()) {
      detail::ShowTooltip(field.availability.disabled_reason);
    }
    ImGui::PopID();
  }

  void DrawInspector(const ApplicationView &view) {
    if (assets.heading_font() != nullptr) {
      ImGui::PushFont(
          assets.heading_font(),
          CurrentLayoutMetrics().typography.section_heading_font_height);
    }
    ImGui::TextUnformatted(view.inspector.title.c_str());
    if (assets.heading_font() != nullptr) {
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
    const LayoutMetrics metrics = CurrentLayoutMetrics();
    const OperationView &operation = *view.operation;
    ImGui::SetCursorPos(
        ImVec2(metrics.spacing.space04, metrics.spacing.space02));
    const float row_y = ImGui::GetCursorScreenPos().y;
    const auto align_to_row = [row_y, &metrics](const float height) {
      const ImVec2 cursor = ImGui::GetCursorScreenPos();
      ImGui::SetCursorScreenPos(ImVec2(
          cursor.x,
          row_y +
              std::floor((metrics.geometry.compact_target - height) * 0.5f)));
    };
    if (detail::DrawOperationDisclosure(assets,
                                        session.operation_tray_visible)) {
      ToggleLayout(detail::LayoutRegion::OperationTray);
    }
    ImGui::SameLine(0.0f, metrics.spacing.space03);
    align_to_row(ImGui::GetTextLineHeight());
    StatusText({.label = operation.title, .status = ToStatus(operation.tone)});
    if (!operation.indeterminate) {
      ImGui::SameLine(0.0f, metrics.spacing.space03);
      align_to_row(metrics.geometry.progress_height);
      ProgressBar({
          .id = "operation-progress",
          .label = operation.title,
          .value = std::clamp(operation.progress, 0.0f, 1.0f),
          .status = ToStatus(operation.tone),
          .size = {.x = 180.0f},
      });
    }
    for (const CommandView &command : operation.commands) {
      ImGui::SameLine(0.0f, metrics.spacing.space03);
      align_to_row(metrics.geometry.compact_target);
      const ButtonResult result = Button({
          .id = command.id.value,
          .label = command.label,
          .tooltip = command.tooltip,
          .variant = ToButtonVariant(command.variant),
          .availability = ToAvailability(command.availability),
          .size = {.x = 0.0f, .y = 24.0f},
      });
      if (result.activated) {
        EmitCommand(view.revision, command);
      }
    }
  }

  void DrawStatusBar(const ApplicationView &view) {
    if (view.status_items.empty()) {
      return;
    }
    const LayoutMetrics metrics = CurrentLayoutMetrics();
    ImGui::SetCursorPos(
        ImVec2(metrics.spacing.space04, metrics.spacing.space02));
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
      invoke(CommandId::OpenProject);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
      invoke(io.KeyShift ? CommandId::SaveProjectAs : CommandId::SaveProject);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_I, false)) {
      invoke(CommandId::ImportFiles);
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
                          const UiEnvironment &environment) {
  impl_->asset_root = asset_root;
  return impl_->assets.Load(asset_root, environment);
}

AssetLoadReport
ApplicationUi::UpdateEnvironment(const UiEnvironment &environment) {
  if (impl_->asset_root.empty()) {
    return {
        .used_fallback_font = true,
        .messages =
            {"Fancy UI must be initialized before its environment is updated"},
    };
  }
  return impl_->assets.Load(impl_->asset_root, environment);
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
  impl_->assets.InstallPendingIcons();
  if (impl_->session.active_destination != view.active_destination) {
    impl_->session.ActivateDestination(view.active_destination);
  }

  ApplyTheme(ResolveTheme(view.theme_mode), impl_->assets.ui_environment());
  const bool operation_available = view.operation.has_value();
  const detail::ApplicationChromeCallbacks chrome_callbacks =
      impl_->ChromeCallbacks(view);
  impl_->chrome.DrawApplicationBar(view.application_bar,
                                   impl_->ChromeLayout(operation_available),
                                   chrome_callbacks);
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
                              [this, &view, &chrome_callbacks]() {
                                impl_->chrome.DrawContextToolbar(
                                    view.context_toolbar, chrome_callbacks);
                              },
                          .zero_padding = true},
      .activity_rail = {.id = "activity-rail",
                        .draw = [this,
                                 &view]() { impl_->DrawActivityRail(view); },
                        .zero_padding = true},
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
                          .visible = view.operation.has_value(),
                          .zero_padding = true},
      .status_bar = {.id = "status-bar",
                     .draw = [this, &view]() { impl_->DrawStatusBar(view); },
                     .zero_padding = true},
  };
  const shell::ApplicationShellResult shell_result =
      shell::Application(shell_spec, shell_state);
  impl_->session.explorer_width = shell_result.state.explorer_width;
  impl_->session.inspector_width = shell_result.state.inspector_width;
  impl_->session.operation_tray_height =
      shell_result.state.operation_tray_height;
  impl_->layout_changed = impl_->layout_changed || shell_result.layout_changed;
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
