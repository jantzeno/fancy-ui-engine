#include "fancy_ui/components/operation_disclosure.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

OperationDisclosureResult
OperationDisclosure(const OperationDisclosureSpec &spec) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const SemanticPalette &palette = CurrentPalette();
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  ImGui::PushID(detail::Owned(spec.id).c_str());
  detail::BeginAvailability(spec.availability);
  const bool activated = ImGui::InvisibleButton(
      "##operation-disclosure",
      ImVec2(metrics.geometry.compact_target, metrics.geometry.compact_target),
      ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (!disabled && (interaction.hovered || interaction.active)) {
    draw_list->AddRectFilled(minimum, maximum,
                             ImGui::GetColorU32(ToImVec4(
                                 interaction.active ? palette.control_pressed
                                                    : palette.control_hover)),
                             metrics.geometry.control_radius);
  }
  const ColorRgba foreground =
      disabled ? palette.text_disabled : palette.text_secondary;
  const float icon_size = metrics.geometry.icon;
  const Rect bounds{
      .minimum = {.x = (minimum.x + maximum.x - icon_size) * 0.5f,
                  .y = (minimum.y + maximum.y - icon_size) * 0.5f},
      .maximum = {.x = (minimum.x + maximum.x + icon_size) * 0.5f,
                  .y = (minimum.y + maximum.y + icon_size) * 0.5f},
  };
  if (spec.icon) {
    spec.icon(bounds, foreground);
  } else {
    const ImU32 color = ImGui::GetColorU32(ToImVec4(foreground));
    if (spec.expanded) {
      draw_list->AddTriangleFilled(
          ImVec2(bounds.minimum.x, bounds.minimum.y + icon_size * 0.25f),
          ImVec2(bounds.maximum.x, bounds.minimum.y + icon_size * 0.25f),
          ImVec2((bounds.minimum.x + bounds.maximum.x) * 0.5f,
                 bounds.maximum.y - icon_size * 0.2f),
          color);
    } else {
      draw_list->AddTriangleFilled(
          ImVec2(bounds.minimum.x + icon_size * 0.25f, bounds.minimum.y),
          ImVec2(bounds.maximum.x - icon_size * 0.2f,
                 (bounds.minimum.y + bounds.maximum.y) * 0.5f),
          ImVec2(bounds.minimum.x + icon_size * 0.25f, bounds.maximum.y),
          color);
    }
  }
  detail::DrawFocusRing(interaction);
  if (interaction.hovered || interaction.focused) {
    detail::ShowTooltip(spec.expanded ? "Collapse operation"
                                      : "Expand operation");
  }
  detail::EndAvailability(spec.availability, {});
  ImGui::PopID();

  OperationDisclosureResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = activated && !disabled;
  result.expanded = result.changed ? !spec.expanded : spec.expanded;
  return result;
}

} // namespace fancy_ui
