#include "internal/operation_disclosure.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>

#include <cmath>
#include <format>
#include <numbers>

namespace fancy_ui::detail {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

bool DrawOperationDisclosure(UiAssetAtlas &assets, const bool expanded,
                             const bool available) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float target = metrics.geometry.compact_target;
  const float icon_size = metrics.geometry.icon;

  if (!available) {
    ImGui::BeginDisabled();
  }
  ImGui::InvisibleButton("##operation-disclosure", ImVec2(target, target),
                         ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = CaptureInteraction();
  const bool activated = available && ImGui::IsItemActivated();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();

  const SemanticPalette &palette = CurrentPalette();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (interaction.active || interaction.hovered) {
    const ColorRgba fill =
        interaction.active ? palette.control_pressed : palette.control_hover;
    draw_list->AddRectFilled(minimum, maximum,
                             ImGui::GetColorU32(ToImVec4(fill)),
                             metrics.geometry.control_radius);
  }
  const float icon_x = std::floor((minimum.x + maximum.x - icon_size) * 0.5f);
  const float icon_y = std::floor((minimum.y + maximum.y - icon_size) * 0.5f);
  static_cast<void>(assets.DrawIcon(
      "chevron-down", steppenface::UiIconSize::Small16,
      {.minimum = {.x = icon_x, .y = icon_y},
       .maximum = {.x = icon_x + icon_size, .y = icon_y + icon_size}},
      available ? palette.text_primary : palette.text_disabled,
      expanded ? 0.0f : -std::numbers::pi_v<float> * 0.5f));
  DrawFocusRing(interaction);
  if (interaction.hovered ||
      (interaction.focused && ImGui::GetIO().NavVisible)) {
    ShowTooltip(
        std::format("{} operation details", expanded ? "Hide" : "Show"));
  }
  if (!available) {
    ImGui::EndDisabled();
  }
  return activated;
}

} // namespace fancy_ui::detail
