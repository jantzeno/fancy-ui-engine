#include "fancy_ui/steppenface/application_ui.hpp"

#include "fancy_ui/components/button.hpp"
#include "fancy_ui/components/checkbox.hpp"
#include "fancy_ui/components/checked_multiselect.hpp"
#include "fancy_ui/components/color_picker_popup.hpp"
#include "fancy_ui/components/color_swatch.hpp"
#include "fancy_ui/components/context_menu.hpp"
#include "fancy_ui/components/duration_input.hpp"
#include "fancy_ui/components/explorer_search.hpp"
#include "fancy_ui/components/hierarchy_row.hpp"
#include "fancy_ui/components/hierarchy_tree.hpp"
#include "fancy_ui/components/information_tree.hpp"
#include "fancy_ui/components/metric_row.hpp"
#include "fancy_ui/components/navigation_item.hpp"
#include "fancy_ui/components/numeric_input.hpp"
#include "fancy_ui/components/progress_bar.hpp"
#include "fancy_ui/components/renamable_select.hpp"
#include "fancy_ui/components/rotation_compass.hpp"
#include "fancy_ui/components/section.hpp"
#include "fancy_ui/components/segmented_control.hpp"
#include "fancy_ui/components/select.hpp"
#include "fancy_ui/components/slider.hpp"
#include "fancy_ui/components/status_card.hpp"
#include "fancy_ui/components/status_text.hpp"
#include "fancy_ui/components/text_input.hpp"
#include "fancy_ui/components/value_display.hpp"
#include "fancy_ui/components/visibility_toggle.hpp"
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
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
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

Validation ToValidation(const ValidationView &validation) {
  return {
      .invalid = validation.invalid,
      .message = validation.message,
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

bool MatchesQuery(const HierarchyRowView &row, const std::string &query) {
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

template <typename Row>
std::vector<std::size_t> SubtreeEnds(const std::vector<Row> &rows) {
  std::vector<std::size_t> ends(rows.size(), rows.size());
  std::vector<std::size_t> ancestors;
  for (std::size_t index = 0; index < rows.size(); ++index) {
    while (!ancestors.empty() &&
           rows[ancestors.back()].depth >= rows[index].depth) {
      ends[ancestors.back()] = index;
      ancestors.pop_back();
    }
    ancestors.push_back(index);
  }
  return ends;
}

std::vector<bool> ExplorerMatches(const std::vector<HierarchyRowView> &rows,
                                  const std::string &query) {
  if (query.empty()) {
    std::vector<bool> visible;
    visible.reserve(rows.size());
    for (const HierarchyRowView &row : rows) {
      visible.push_back(row.availability.visible);
    }
    return visible;
  }
  int maximum_depth = 0;
  for (const HierarchyRowView &row : rows) {
    maximum_depth = std::max(maximum_depth, row.depth);
  }
  std::vector<bool> descendant_match(
      static_cast<std::size_t>(maximum_depth + 2), false);
  std::vector<bool> visible(rows.size(), false);
  for (std::size_t reverse = rows.size(); reverse > 0; --reverse) {
    const std::size_t index = reverse - 1;
    const HierarchyRowView &row = rows[index];
    const std::size_t depth = static_cast<std::size_t>(std::max(row.depth, 0));
    visible[index] = row.availability.visible &&
                     (MatchesQuery(row, query) || descendant_match[depth + 1]);
    descendant_match[depth + 1] = false;
    descendant_match[depth] = descendant_match[depth] || visible[index];
  }
  return visible;
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

  Impl() : chrome(*assets) {}
  explicit Impl(detail::UiAssetAtlas &shared_assets)
      : assets(&shared_assets), chrome(*assets) {}

  SessionState session;
  std::filesystem::path asset_root;
  detail::UiAssetAtlas owned_assets;
  detail::UiAssetAtlas *assets = &owned_assets;
  detail::ApplicationChrome chrome;
  std::vector<UiIntent> intents;
  std::unordered_map<std::string, FieldValue> field_drafts;
  std::unordered_map<std::string, RenamableSelectState> rename_states;
  std::unordered_map<std::string, ColorPickerState> color_states;
  std::unordered_map<std::string, ContextMenuState> context_menu_states;
  std::unordered_set<std::string> live_transient_ids;
  bool navigation_changed = false;
  bool layout_changed = false;

  void EmitCommand(const std::uint64_t revision, const CommandView &command) {
    if (command.availability.visible && command.availability.enabled &&
        !command.availability.busy) {
      intents.emplace_back(InvokeCommand{.revision = revision,
                                         .control = command.id,
                                         .command = command.command});
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
    });
  }

  void EmitEdit(const std::uint64_t revision, const EditBindingView &binding,
                FieldValue value, const Availability &availability) {
    EmitEdit(revision, ControlActionView{
                           .field = binding.field,
                           .value = std::move(value),
                           .target = binding.target,
                           .availability = availability,
                       });
  }

  void PruneTransientState() {
    const auto prune = [this](auto &states) {
      std::erase_if(states, [this](const auto &entry) {
        return !live_transient_ids.contains(entry.first);
      });
    };
    prune(field_drafts);
    prune(rename_states);
    prune(color_states);
    prune(context_menu_states);
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
    return assets->DrawIcon(
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
    detail::PushMenuPopupStyle();
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
    detail::PopMenuPopupStyle();
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

  struct ContextItemStorage {
    ContextMenuItemSpec spec;
    std::vector<ContextItemStorage> children;
    std::vector<ContextMenuItemSpec> child_specs;
  };

  ContextItemStorage BuildContextItem(const MenuItemView &item) {
    ContextItemStorage storage;
    storage.children.reserve(item.children.size());
    for (const MenuItemView &child : item.children) {
      storage.children.push_back(BuildContextItem(child));
    }
    storage.child_specs.reserve(storage.children.size());
    for (const ContextItemStorage &child : storage.children) {
      storage.child_specs.push_back(child.spec);
    }
    const Availability *availability = nullptr;
    if (item.command.has_value()) {
      availability = &item.command->availability;
    } else if (item.action.has_value()) {
      availability = &item.action->availability;
    }
    storage.spec = {
        .id = item.id.value,
        .label = item.label,
        .shortcut = item.command.has_value()
                        ? std::string_view(item.command->shortcut)
                        : std::string_view{},
        .tooltip = item.command.has_value()
                       ? std::string_view(item.command->tooltip)
                       : std::string_view{},
        .kind = item.kind == MenuItemKind::Separator
                    ? ContextMenuItemKind::Separator
                : item.kind == MenuItemKind::Submenu
                    ? ContextMenuItemKind::Submenu
                    : ContextMenuItemKind::Command,
        .selected = item.selected,
        .availability = availability == nullptr ? fancy_ui::Availability{}
                                                : ToAvailability(*availability),
        .children = storage.child_specs,
    };
    return storage;
  }

  const MenuItemView *FindMenuItem(const std::vector<MenuItemView> &items,
                                   const std::string &id) const {
    for (const MenuItemView &item : items) {
      if (item.id.value == id) {
        return &item;
      }
      if (const MenuItemView *child = FindMenuItem(item.children, id);
          child != nullptr) {
        return child;
      }
    }
    return nullptr;
  }

  void DrawContextMenu(const ApplicationView &view, const ContextMenuView &menu,
                       const bool request_open) {
    live_transient_ids.insert(menu.id.value);
    std::vector<ContextItemStorage> storage;
    storage.reserve(menu.items.size());
    for (const MenuItemView &item : menu.items) {
      storage.push_back(BuildContextItem(item));
    }
    std::vector<ContextMenuItemSpec> specs;
    specs.reserve(storage.size());
    for (const ContextItemStorage &item : storage) {
      specs.push_back(item.spec);
    }
    ContextMenuResult result = ContextMenu(
        {.id = menu.id.value, .items = specs, .request_open = request_open},
        context_menu_states[menu.id.value]);
    if (!result.activated_id.has_value()) {
      return;
    }
    const MenuItemView *item = FindMenuItem(menu.items, *result.activated_id);
    if (item == nullptr) {
      return;
    }
    if (item->command.has_value()) {
      EmitCommand(view.revision, *item->command);
    } else if (item->action.has_value()) {
      EmitEdit(view.revision, *item->action);
    }
  }

  void DrawExplorerNode(const ApplicationView &view,
                        const std::vector<bool> &matches,
                        const std::vector<std::size_t> &subtree_ends,
                        const std::string &query, const std::size_t index,
                        HierarchyTree &tree) {
    const std::vector<HierarchyRowView> &rows = view.panel.explorer.rows;
    if (!matches[index]) {
      return;
    }

    const HierarchyRowView &row = rows[index];
    const std::size_t subtree_end = subtree_ends[index];
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
        tree,
        {
            .id = row.id.value,
            .label = row.label,
            .metadata = row.secondary_label,
            .tooltip = tooltip,
            .expandable = expandable,
            .expanded = expanded,
            .selected = row.selected,
            .status = ToStatus(row.tone),
            .leading_icon =
                row.icon.empty() ? IconPainter{} : assets->Painter(row.icon),
            .color = row.color_edit.has_value() ? row.color
                                                : std::optional<ColorRgba>{},
            .action_icon = row.context_menu.has_value()
                               ? assets->Painter("more")
                               : IconPainter{},
            .visibility = row.visibility_edit.has_value()
                              ? row.visibility
                              : std::optional<ToggleState>{},
            .visible_icon = assets->Painter("visibility"),
            .hidden_icon = assets->Painter("visibility-off"),
            .availability = ToAvailability(row.availability),
        });
    if (result.expansion_changed && query.empty()) {
      expansion->second = result.expanded;
    }
    if (result.activated) {
      intents.emplace_back(ChangeSelection{
          .revision = view.revision,
          .source = row.id,
          .entity = row.entity,
          .additive = result.additive,
          .range = result.range,
      });
    }
    if (result.visibility_changed && row.visibility_edit.has_value()) {
      EmitEdit(view.revision, *row.visibility_edit, result.visibility,
               row.availability);
    }
    if (row.color.has_value() && row.color_edit.has_value()) {
      const std::string color_id = row.id.value + ".color";
      live_transient_ids.insert(color_id);
      ColorPickerPopupResult color = ColorPickerPopup(
          {
              .id = color_id,
              .title = "Edit color",
              .value = *row.color,
              .request_open = result.color_activated,
          },
          color_states[color_id]);
      if (color.committed) {
        EmitEdit(view.revision, *row.color_edit, color.value, row.availability);
      }
    }
    if (row.context_menu.has_value()) {
      DrawContextMenu(view, *row.context_menu, result.action_activated);
    }

    if (!expandable || !result.expanded) {
      return;
    }
    std::size_t child = index + 1;
    while (child < subtree_end) {
      DrawExplorerNode(view, matches, subtree_ends, query, child, tree);
      child = subtree_ends[child];
    }
    tree.Pop();
  }

  void DrawExplorer(const ApplicationView &view) {
    if (assets->heading_font() != nullptr) {
      ImGui::PushFont(
          assets->heading_font(),
          CurrentLayoutMetrics().typography.section_heading_font_height);
    }
    ImGui::TextUnformatted(view.panel.explorer.title.c_str());
    if (assets->heading_font() != nullptr) {
      ImGui::PopFont();
    }

    std::string &query = session.explorer_queries[session.active_destination];
    const ExplorerSearchResult search = ExplorerSearch({
        .id = view.panel.explorer.search.id.value,
        .placeholder = view.panel.explorer.search.placeholder,
        .query = query,
        .availability = ToAvailability(view.panel.explorer.search.availability),
    });
    if (search.changed) {
      query = search.query;
    }

    for (const CommandView &command : view.panel.explorer.commands) {
      DrawCommandButton(view.revision, command, true);
      ImGui::SameLine();
    }
    if (!view.panel.explorer.commands.empty()) {
      ImGui::NewLine();
    }
    if (!view.panel.explorer.tree_label.empty()) {
      detail::DrawSecondaryText(view.panel.explorer.tree_label);
    }

    HierarchyTree tree(
        {.section_font = NativeFontHandle(assets->heading_font())});
    const std::vector<bool> matches =
        ExplorerMatches(view.panel.explorer.rows, query);
    const std::vector<std::size_t> subtree_ends =
        SubtreeEnds(view.panel.explorer.rows);
    std::size_t root = 0;
    while (root < view.panel.explorer.rows.size()) {
      DrawExplorerNode(view, matches, subtree_ends, query, root, tree);
      root = subtree_ends[root];
    }
    if (!view.panel.explorer.footer.empty()) {
      detail::DrawSecondaryText(view.panel.explorer.footer);
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
    live_transient_ids.insert(field.id.value);
    const fancy_ui::Availability availability =
        ToAvailability(field.availability);
    const Validation validation = ToValidation(field.validation);
    const bool stacked_label =
        FieldLabelLayoutFor(field.content) == FieldLabelLayout::Stacked;
    const std::string_view control_label =
        stacked_label ? std::string_view{} : std::string_view{field.label};
    if (stacked_label && !field.label.empty()) {
      const LayoutMetrics metrics = CurrentLayoutMetrics();
      ImGui::PushFont(nullptr, metrics.typography.body_font_height);
      const ImVec2 item_spacing = ImGui::GetStyle().ItemSpacing;
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                          ImVec2(item_spacing.x, 0.0f));
      detail::DrawSecondaryText(field.label);
      ImGui::Dummy(ImVec2(0.0f, metrics.spacing.space02));
      ImGui::PopStyleVar();
      ImGui::PopFont();
    }
    const auto commit = [this, &view, &field](FieldValue value) {
      if (field.edit.has_value()) {
        EmitEdit(view.revision, *field.edit, std::move(value),
                 field.availability);
      }
      field_drafts.erase(field.id.value);
    };
    const auto draft = [this, &field](FieldValue value) {
      field_drafts.insert_or_assign(field.id.value, std::move(value));
    };
    const auto draft_value = [this, &field](const auto &fallback) {
      using Value = std::decay_t<decltype(fallback)>;
      const auto found = field_drafts.find(field.id.value);
      if (found != field_drafts.end()) {
        if (const Value *value = std::get_if<Value>(&found->second)) {
          return *value;
        }
      }
      return fallback;
    };

    std::visit(
        [this, &view, &field, &availability, &validation, &control_label,
         &commit, &draft, &draft_value](const auto &content) {
          using Content = std::decay_t<decltype(content)>;
          if constexpr (std::is_same_v<Content, TextFieldView>) {
            const std::string value = draft_value(content.value);
            const TextInputResult result = TextInput({
                .id = field.id.value,
                .label = control_label,
                .tooltip = field.tooltip,
                .value = value,
                .placeholder = content.placeholder,
                .capacity = content.capacity,
                .availability = availability,
                .validation = validation,
            });
            if (result.changed) {
              draft(result.value);
            }
            if (result.committed) {
              commit(result.value);
            } else if (result.cancelled) {
              field_drafts.erase(field.id.value);
            }
          } else if constexpr (std::is_same_v<Content, NumericFieldView>) {
            const double value = draft_value(content.value);
            const NumericInputResult result = NumericInput({
                .id = field.id.value,
                .label = control_label,
                .tooltip = field.tooltip,
                .unit = content.unit,
                .value = value,
                .minimum = content.minimum,
                .maximum = content.maximum,
                .format = content.format,
                .availability = availability,
                .validation = validation,
            });
            if (result.changed) {
              draft(result.value);
            }
            if (result.committed) {
              if (content.integral) {
                commit(static_cast<std::int64_t>(std::llround(result.value)));
              } else {
                commit(result.value);
              }
            } else if (result.cancelled) {
              field_drafts.erase(field.id.value);
            }
          } else if constexpr (std::is_same_v<Content, SelectFieldView> ||
                               std::is_same_v<Content,
                                              RenamableSelectFieldView>) {
            std::vector<SelectOption> options;
            options.reserve(content.options.size());
            std::size_t selected_index = 0;
            for (std::size_t index = 0; index < content.options.size();
                 ++index) {
              const ChoiceOptionView &option = content.options[index];
              options.push_back({
                  .id = option.id.value,
                  .label = option.label,
                  .tooltip = option.tooltip,
                  .availability = ToAvailability(option.availability),
              });
              if (option.id == content.selected) {
                selected_index = index;
              }
            }
            if (options.empty()) {
              static_cast<void>(ValueDisplay({.id = field.id.value,
                                              .label = control_label,
                                              .value = "—",
                                              .tooltip = field.tooltip}));
            } else if constexpr (std::is_same_v<Content, SelectFieldView>) {
              const SelectResult result = Select({
                  .id = field.id.value,
                  .label = control_label,
                  .tooltip = field.tooltip,
                  .options = options,
                  .selected_index = selected_index,
                  .availability = availability,
                  .validation = validation,
              });
              if (result.changed) {
                commit(content.options[result.selected_index].id);
              }
            } else {
              RenamableSelectState &state = rename_states[field.id.value];
              const RenamableSelectResult result = RenamableSelect(
                  {
                      .id = field.id.value,
                      .label = control_label,
                      .tooltip = field.tooltip,
                      .options = options,
                      .selected_index = selected_index,
                      .availability = availability,
                      .rename_availability = availability,
                      .validation = validation,
                  },
                  state);
              if (result.selection_changed) {
                commit(content.options[result.selected_index].id);
              }
              if (result.committed) {
                EmitEdit(view.revision, content.rename, result.value,
                         field.availability);
              }
            }
          } else if constexpr (std::is_same_v<Content, MultiselectFieldView>) {
            std::vector<CheckedMultiselectOption> options;
            options.reserve(content.options.size());
            for (const ToggleOptionView &option : content.options) {
              options.push_back({
                  .id = option.option.id.value,
                  .label = option.option.label,
                  .state = option.state,
                  .availability = ToAvailability(option.option.availability),
              });
            }
            const CheckedMultiselectResult result = CheckedMultiselect({
                .id = field.id.value,
                .label = control_label,
                .summary = content.summary,
                .tooltip = field.tooltip,
                .options = options,
                .availability = availability,
            });
            if (result.changed && result.option_id.has_value()) {
              commit(ChoiceToggleValue{
                  .option = {.value = *result.option_id},
                  .state = result.state,
              });
            }
          } else if constexpr (std::is_same_v<Content, SegmentedFieldView>) {
            std::vector<ChoiceSpec> choices;
            choices.reserve(content.options.size());
            std::size_t selected_index = 0;
            for (std::size_t index = 0; index < content.options.size();
                 ++index) {
              const ChoiceOptionView &option = content.options[index];
              choices.push_back({
                  .id = option.id.value,
                  .label = option.label,
                  .tooltip = option.tooltip,
                  .availability = ToAvailability(option.availability),
              });
              if (option.id == content.selected) {
                selected_index = index;
              }
            }
            if (!choices.empty()) {
              ImGui::PushFont(
                  nullptr, CurrentLayoutMetrics().typography.body_font_height);
              const detail::FieldLayout layout =
                  detail::BeginFieldLayout(control_label);
              const SegmentedControlResult result = SegmentedControl({
                  .id = field.id.value,
                  .choices = choices,
                  .selected_index = selected_index,
                  .width = content.width,
              });
              detail::EndFieldLayout(layout, validation);
              ImGui::PopFont();
              if (result.changed) {
                commit(content.options[result.selected_index].id);
              }
            }
          } else if constexpr (std::is_same_v<Content, CheckboxFieldView>) {
            const std::string value =
                !content.value.empty()              ? content.value
                : content.state == ToggleState::On  ? "Enabled"
                : content.state == ToggleState::Off ? "Disabled"
                                                    : "Mixed";
            ImGui::PushFont(nullptr,
                            CurrentLayoutMetrics().typography.body_font_height);
            const detail::FieldLayout layout =
                detail::BeginFieldLayout(control_label);
            const CheckboxResult result = Checkbox({
                .id = field.id.value,
                .label = value,
                .tooltip = field.tooltip,
                .state = content.state,
                .on_icon = content.on_icon.empty()
                               ? IconPainter{}
                               : assets->Painter(content.on_icon),
                .off_icon = content.off_icon.empty()
                                ? IconPainter{}
                                : assets->Painter(content.off_icon),
                .show_checkbox = content.show_checkbox,
                .availability = availability,
                .validation = validation,
            });
            detail::EndFieldLayout(layout, {});
            ImGui::PopFont();
            if (result.changed) {
              commit(result.state);
            }
          } else if constexpr (std::is_same_v<Content, VisibilityFieldView>) {
            const VisibilityToggleResult result = VisibilityToggle({
                .id = field.id.value,
                .label = control_label,
                .tooltip = field.tooltip,
                .state = content.state,
                .visible_icon = assets->Painter("visibility"),
                .hidden_icon = assets->Painter("visibility-off"),
                .availability = availability,
            });
            if (result.changed) {
              commit(result.state);
            }
          } else if constexpr (std::is_same_v<Content, SliderFieldView>) {
            const double canonical = static_cast<double>(content.value);
            const float value = static_cast<float>(draft_value(canonical));
            const SliderResult result = Slider({
                .id = field.id.value,
                .label = control_label,
                .tooltip = field.tooltip,
                .unit = content.unit,
                .value = value,
                .minimum = content.minimum,
                .maximum = content.maximum,
                .format = content.format,
                .availability = availability,
                .validation = validation,
            });
            if (result.changed) {
              draft(static_cast<double>(result.value));
            }
            if (result.committed) {
              commit(static_cast<double>(result.value));
            }
          } else if constexpr (std::is_same_v<Content,
                                              RotationCompassFieldView>) {
            const std::int64_t canonical = content.count;
            const int count = static_cast<int>(draft_value(canonical));
            const RotationCompassResult result = RotationCompass({
                .id = field.id.value,
                .label = control_label,
                .tooltip = field.tooltip,
                .count = count,
                .inherited = content.inherited,
                .availability = availability,
            });
            if (result.changed) {
              draft(static_cast<std::int64_t>(result.count));
            }
            if (result.committed) {
              commit(static_cast<std::int64_t>(result.count));
            }
          } else if constexpr (std::is_same_v<Content, DurationFieldView>) {
            const DurationValue value = draft_value(content.value);
            const DurationResult result = Duration({
                .id = field.id.value,
                .label = control_label,
                .tooltip = field.tooltip,
                .hours = value.hours,
                .minutes = value.minutes,
                .availability = availability,
                .validation = validation,
            });
            const DurationValue edited{.hours = result.hours,
                                       .minutes = result.minutes};
            if (result.changed) {
              draft(edited);
            }
            if (result.committed) {
              commit(edited);
            } else if (result.cancelled) {
              field_drafts.erase(field.id.value);
            }
          } else if constexpr (std::is_same_v<Content, ColorFieldView>) {
            const ColorRgba value = draft_value(content.value);
            const std::span<const ColorRgba> colors =
                content.colors.empty()
                    ? std::span<const ColorRgba>(&value, 1)
                    : std::span<const ColorRgba>(content.colors);
            const ColorSwatchResult result = ColorSwatch(
                {
                    .id = field.id.value,
                    .label = control_label,
                    .tooltip = field.tooltip,
                    .picker_title = field.label,
                    .value = value,
                    .colors = colors,
                    .show_alpha = content.show_alpha,
                    .availability = availability,
                },
                color_states[field.id.value]);
            if (result.changed) {
              draft(result.value);
            }
            if (result.committed) {
              commit(result.value);
            } else if (result.cancelled) {
              field_drafts.erase(field.id.value);
            }
          } else if constexpr (std::is_same_v<Content, ValueFieldView>) {
            static_cast<void>(ValueDisplay({
                .id = field.id.value,
                .label = control_label,
                .value = content.value,
                .tooltip = field.tooltip,
                .mixed = content.mixed,
            }));
          } else if constexpr (std::is_same_v<Content, ButtonFieldView>) {
            DrawCommandButton(view.revision, content.command, false);
          }
        },
        field.content);
    if (!field.help.empty()) {
      detail::DrawSecondaryText(field.help);
    }
  }

  void DrawInformationNode(const ApplicationView &view,
                           const std::vector<InformationTreeRowView> &rows,
                           const std::vector<std::size_t> &subtree_ends,
                           const std::size_t index, InformationTree &tree) {
    const InformationTreeRowView &row = rows[index];
    if (!row.availability.visible) {
      return;
    }
    std::vector<MetricValue> metrics;
    metrics.reserve(row.metrics.size());
    for (const MetricValueView &metric : row.metrics) {
      metrics.push_back({
          .label = metric.label,
          .value = metric.value,
          .wide = metric.wide,
          .stacked = metric.stacked,
      });
    }
    std::vector<ButtonSpec> actions;
    actions.reserve(row.actions.size());
    for (const CommandView &command : row.actions) {
      actions.push_back({
          .id = command.id.value,
          .label = command.label,
          .tooltip = command.tooltip,
          .variant = ToButtonVariant(command.variant),
          .availability = ToAvailability(command.availability),
      });
    }
    auto [expansion, inserted] =
        session.explorer_expanded_rows.try_emplace(row.id.value, row.expanded);
    static_cast<void>(inserted);
    const std::size_t subtree_end = subtree_ends[index];
    const bool expandable = row.expandable || subtree_end > index + 1;
    const InformationTreeRowResult result = InformationTreeRow(
        tree, {
                  .id = row.id.value,
                  .label = row.label,
                  .metadata = row.metadata,
                  .expandable = expandable,
                  .expanded = expandable && expansion->second,
                  .selected = row.selected,
                  .highlighted = row.highlighted,
                  .status = ToStatus(row.tone),
                  .metrics = metrics,
                  .actions = actions,
                  .visibility = row.visibility_edit.has_value()
                                    ? row.visibility
                                    : std::optional<ToggleState>{},
                  .visible_icon = assets->Painter("visibility"),
                  .hidden_icon = assets->Painter("visibility-off"),
                  .availability = ToAvailability(row.availability),
              });
    if (result.expansion_changed) {
      expansion->second = result.expanded;
    }
    if (result.activated) {
      intents.emplace_back(ChangeSelection{
          .revision = view.revision,
          .source = row.id,
          .entity = row.entity,
          .additive = result.additive,
          .range = result.range,
      });
    }
    if (result.visibility_changed && row.visibility_edit.has_value()) {
      EmitEdit(view.revision, *row.visibility_edit, result.visibility,
               row.availability);
    }
    if (result.activated_action.has_value() &&
        *result.activated_action < row.actions.size()) {
      EmitCommand(view.revision, row.actions[*result.activated_action]);
    }
    if (!expandable || !result.expanded) {
      return;
    }
    std::size_t child = index + 1;
    while (child < subtree_end) {
      DrawInformationNode(view, rows, subtree_ends, child, tree);
      child = subtree_ends[child];
    }
    tree.Pop();
  }

  void DrawInspector(const ApplicationView &view) {
    if (assets->heading_font() != nullptr) {
      ImGui::PushFont(
          assets->heading_font(),
          CurrentLayoutMetrics().typography.section_heading_font_height);
    }
    ImGui::TextUnformatted(view.panel.inspector.title.c_str());
    if (assets->heading_font() != nullptr) {
      ImGui::PopFont();
    }
    if (!view.panel.inspector.subtitle.empty()) {
      detail::DrawSecondaryText(view.panel.inspector.subtitle);
    }
    if (!view.panel.inspector.scope.empty()) {
      StatusText({.label = view.panel.inspector.scope,
                  .status = SemanticStatus::Information});
    }
    if (!view.panel.inspector.note.empty()) {
      ImGui::TextWrapped("%s", view.panel.inspector.note.c_str());
    }
    if (view.panel.inspector.sections.empty()) {
      ImGui::TextWrapped("%s", view.panel.inspector.empty_message.c_str());
    }
    for (std::size_t index = 0; index < view.panel.inspector.sections.size();
         ++index) {
      const SectionView &section = view.panel.inspector.sections[index];
      auto [collapsed, inserted] = session.collapsed_sections.try_emplace(
          section.id.value, !section.default_open);
      static_cast<void>(inserted);
      std::optional<ButtonSpec> header_action;
      if (section.header_command.has_value()) {
        const CommandView &command = *section.header_command;
        header_action = ButtonSpec{
            .id = command.id.value,
            .label = command.label,
            .tooltip = command.tooltip,
            .variant = ToButtonVariant(command.variant),
            .availability = ToAvailability(command.availability),
        };
      }
      const SectionResult result = BeginSection({
          .id = section.id.value,
          .heading = section.heading,
          .summary = section.summary,
          .open = !collapsed->second,
          .focused = section.focused,
          .separated = index > 0,
          .header_action = header_action,
      });
      if (result.open_changed) {
        collapsed->second = !result.open;
      }
      if (result.header_action_activated &&
          section.header_command.has_value()) {
        EmitCommand(view.revision, *section.header_command);
      }
      if (result.open) {
        if (!section.information_rows.empty()) {
          InformationTree tree(
              {.section_font = NativeFontHandle(assets->heading_font())});
          const std::vector<std::size_t> subtree_ends =
              SubtreeEnds(section.information_rows);
          std::size_t root = 0;
          while (root < section.information_rows.size()) {
            DrawInformationNode(view, section.information_rows, subtree_ends,
                                root, tree);
            root = subtree_ends[root];
          }
        }
        const ImVec2 item_spacing = ImGui::GetStyle().ItemSpacing;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                            ImVec2(item_spacing.x, 0.0f));
        for (const FieldView &field : section.fields) {
          DrawField(view, field);
        }
        ImGui::PopStyleVar();
        if (section.status.has_value()) {
          StatusCard({
              .id = section.status->id.value,
              .title = section.status->title,
              .message = section.status->message,
              .status = ToStatus(section.status->tone),
              .icon = section.status->icon.empty()
                          ? IconPainter{}
                          : assets->Painter(section.status->icon),
          });
        }
        for (const CommandView &command : section.actions) {
          DrawCommandButton(view.revision, command, false);
        }
      }
      EndSection(result);
    }
    if (view.panel.inspector.primary_command.has_value()) {
      DrawCommandButton(view.revision, *view.panel.inspector.primary_command,
                        false);
    }
    for (const CommandView &command : view.panel.inspector.secondary_commands) {
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
    if (detail::DrawOperationDisclosure(*assets,
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
ApplicationUi::ApplicationUi(detail::UiAssetAtlas &shared_assets)
    : impl_(std::make_unique<Impl>(shared_assets)) {}
ApplicationUi::~ApplicationUi() = default;
ApplicationUi::ApplicationUi(ApplicationUi &&) noexcept = default;
ApplicationUi &ApplicationUi::operator=(ApplicationUi &&) noexcept = default;

AssetLoadReport
ApplicationUi::Initialize(const std::filesystem::path &asset_root,
                          const UiEnvironment &environment) {
  if (impl_->assets != &impl_->owned_assets) {
    return {};
  }
  impl_->asset_root = asset_root;
  return impl_->assets->Load(asset_root, environment);
}

AssetLoadReport
ApplicationUi::UpdateEnvironment(const UiEnvironment &environment) {
  if (impl_->assets != &impl_->owned_assets) {
    return {};
  }
  if (impl_->asset_root.empty()) {
    return {
        .used_fallback_font = true,
        .messages =
            {"Fancy UI must be initialized before its environment is updated"},
    };
  }
  return impl_->assets->Load(impl_->asset_root, environment);
}

const SessionState &ApplicationUi::session() const { return impl_->session; }

void ApplicationUi::SetSession(SessionState session) {
  session.ActivateDestination(session.active_destination);
  impl_->session = std::move(session);
}

FrameResult ApplicationUi::Draw(const ApplicationView &view,
                                const SurfaceBindings &surfaces) {
  impl_->intents.clear();
  impl_->live_transient_ids.clear();
  impl_->navigation_changed = false;
  impl_->layout_changed = false;
  impl_->assets->InstallPendingIcons();
  if (impl_->session.active_destination != view.panel.destination) {
    impl_->session.ActivateDestination(view.panel.destination);
  }

  ApplyTheme(ResolveTheme(view.theme_mode), impl_->assets->ui_environment());
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
                          .padding = 0.0f},
      .activity_rail = {.id = "activity-rail",
                        .draw = [this,
                                 &view]() { impl_->DrawActivityRail(view); },
                        .padding = 0.0f},
      .explorer = {.id = "explorer",
                   .draw = [this, &view]() { impl_->DrawExplorer(view); },
                   .padding = 8.0f},
      .workspace = {.id = "workspace",
                    .draw =
                        [this, &view, &surfaces]() {
                          impl_->DrawWorkspace(view, surfaces);
                        },
                    .padding = 0.0f},
      .inspector = {.id = "inspector",
                    .draw = [this, &view]() { impl_->DrawInspector(view); },
                    .padding = 8.0f},
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
                          .padding = 0.0f},
      .status_bar = {.id = "status-bar",
                     .draw = [this, &view]() { impl_->DrawStatusBar(view); },
                     .padding = 0.0f},
  };
  const shell::ApplicationShellResult shell_result =
      shell::Application(shell_spec, shell_state);
  impl_->session.explorer_width = shell_result.state.explorer_width;
  impl_->session.inspector_width = shell_result.state.inspector_width;
  impl_->session.operation_tray_height =
      shell_result.state.operation_tray_height;
  impl_->layout_changed = impl_->layout_changed || shell_result.layout_changed;
  impl_->HandleShortcuts(view);
  impl_->PruneTransientState();

  ImGui::End();
  ImGui::PopStyleVar();
  return {
      .product_intents = std::move(impl_->intents),
      .backend_requests = {},
      .navigation_changed = impl_->navigation_changed,
      .layout_changed = impl_->layout_changed,
  };
}

FrameResult
ApplicationUi::DrawPanelAudit(const ApplicationView &view,
                              const std::function<void()> &draw_audit_menu) {
  impl_->intents.clear();
  impl_->live_transient_ids.clear();
  impl_->navigation_changed = false;
  impl_->layout_changed = false;
  impl_->assets->InstallPendingIcons();
  if (impl_->session.active_destination != view.panel.destination) {
    impl_->session.ActivateDestination(view.panel.destination);
  }
  impl_->session.explorer_visible = true;
  impl_->session.inspector_visible = true;

  ApplyTheme(ResolveTheme(view.theme_mode), impl_->assets->ui_environment());
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("##steppenface-panel-audit", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  const shell::ApplicationShellState shell_state{
      .explorer_visible = true,
      .inspector_visible = true,
      .operation_tray_visible = false,
      .explorer_width = impl_->session.explorer_width,
      .inspector_width = impl_->session.inspector_width,
  };
  const shell::ApplicationShellSpec shell_spec{
      .application_bar = {.id = "application-bar", .visible = false},
      .context_toolbar = {.id = "context-toolbar", .visible = false},
      .activity_rail = {.id = "activity-rail",
                        .draw = [this,
                                 &view]() { impl_->DrawActivityRail(view); },
                        .padding = 0.0f},
      .explorer = {.id = "explorer",
                   .draw = [this, &view]() { impl_->DrawExplorer(view); },
                   .padding = 8.0f},
      .workspace = {.id = "panel-audit-menu", .draw = draw_audit_menu},
      .inspector = {.id = "inspector",
                    .draw = [this, &view]() { impl_->DrawInspector(view); },
                    .padding = 8.0f},
      .operation_tray = {.id = "operation-tray", .visible = false},
      .operation_strip = {.id = "operation-strip", .visible = false},
      .status_bar = {.id = "status-bar", .visible = false},
  };
  const shell::ApplicationShellResult shell_result =
      shell::Application(shell_spec, shell_state);
  impl_->session.explorer_width = shell_result.state.explorer_width;
  impl_->session.inspector_width = shell_result.state.inspector_width;
  impl_->layout_changed = shell_result.layout_changed;
  impl_->PruneTransientState();

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
