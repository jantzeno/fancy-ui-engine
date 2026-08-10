#include "fancy_ui/components/rotation_compass.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
#include <string>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

std::string CountLabel(const int count) {
  return std::format("{} {}", count, count == 1 ? "rotation" : "rotations");
}

std::string StepLabel(const RotationCompassSpec &spec, const int count) {
  const std::string step =
      "Every " + FormatRotationDegrees(RotationStepDegrees(count));
  return spec.inherited ? "Inherited · " + step : step;
}

} // namespace

int ClampRotationCount(const int value) {
  return std::clamp(value, kRotationCountMinimum, kRotationCountMaximum);
}

double RotationStepDegrees(const int count) {
  return 360.0 / static_cast<double>(ClampRotationCount(count));
}

std::vector<double> EvenlySpacedRotationAngles(const int count) {
  const int clamped = ClampRotationCount(count);
  const double step = RotationStepDegrees(clamped);
  std::vector<double> angles;
  angles.reserve(static_cast<std::size_t>(clamped));
  for (int index = 0; index < clamped; ++index) {
    angles.push_back(step * static_cast<double>(index));
  }
  return angles;
}

std::string FormatRotationDegrees(const double degrees) {
  const double rounded = std::round(degrees);
  if (std::abs(degrees - rounded) < 1e-6) {
    return std::format("{}°", static_cast<int>(rounded));
  }
  return std::format("{:.1f}°", degrees);
}

RotationCompassResult RotationCompass(const RotationCompassSpec &spec) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  ImGui::PushFont(nullptr, metrics.typography.body_font_height);
  const std::string id = detail::Owned(spec.id);
  int count = ClampRotationCount(spec.count);
  const bool disabled = !spec.availability.enabled || spec.availability.busy;
  const SemanticPalette &palette = CurrentPalette();
  const float control_height = Scale(104.0f);
  const float compass_size = Scale(64.0f);
  const float padding = Scale(8.0f);

  ImGui::PushID(id.c_str());
  const ImVec2 item_spacing = ImGui::GetStyle().ItemSpacing;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(item_spacing.x, 0.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(palette.text_secondary));
  ImGui::TextUnformatted(detail::Owned(spec.label).c_str());
  ImGui::PopStyleColor();
  ImGui::Dummy(ImVec2(0.0f, metrics.spacing.space02));
  ImGui::PopStyleVar();
  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        ToImVec4(disabled ? palette.control_disabled_fill
                                          : palette.surface_raised));
  ImGui::PushStyleColor(ImGuiCol_Border,
                        ToImVec4(disabled ? palette.control_disabled_border
                                          : palette.border_strong));
  if (disabled) {
    ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(palette.text_disabled));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ToImVec4(palette.text_disabled));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,
                          ToImVec4(palette.text_disabled));
  }
  const bool child_open =
      ImGui::BeginChild("##rotation-compass", ImVec2(0.0f, control_height),
                        ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);

  bool changed = false;
  bool committed = false;
  InteractionResult interaction;
  if (child_open) {
    const ImVec2 child_minimum = ImGui::GetWindowPos();
    const ImVec2 child_size = ImGui::GetWindowSize();
    const ImVec2 compass_minimum(
        child_minimum.x + child_size.x - compass_size - padding,
        child_minimum.y + (child_size.y - compass_size) * 0.5f);
    const ImVec2 compass_maximum(compass_minimum.x + compass_size,
                                 compass_minimum.y + compass_size);
    const ImVec2 center((compass_minimum.x + compass_maximum.x) * 0.5f,
                        (compass_minimum.y + compass_maximum.y) * 0.5f);
    const float radius = compass_size * 0.5f - Scale(1.0f);
    const float editor_width = std::max(
        Scale(80.0f),
        compass_minimum.x - ImGui::GetCursorScreenPos().x - Scale(10.0f));

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ToImVec4(disabled ? palette.text_disabled : palette.text_primary));
    ImGui::TextUnformatted(CountLabel(count).c_str());
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(palette.text_secondary));
    ImGui::TextWrapped("%s", StepLabel(spec, count).c_str());
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(editor_width);
    detail::BeginAvailability(spec.availability);
    changed = detail::DrawSliderInt("##count", count, kRotationCountMinimum,
                                    kRotationCountMaximum, false, false,
                                    ImGuiSliderFlags_AlwaysClamp);
    interaction = detail::CaptureInteraction();
    const ImVec2 slider_minimum = ImGui::GetItemRectMin();
    const ImVec2 slider_maximum = ImGui::GetItemRectMax();
    committed = ImGui::IsItemDeactivatedAfterEdit();
    detail::DrawFocusRing(interaction, true);
    detail::EndAvailability(spec.availability, spec.tooltip);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const std::string minimum_label = "1";
    const std::string maximum_label = std::to_string(kRotationCountMaximum);
    const ImVec2 maximum_label_size =
        ImGui::CalcTextSize(maximum_label.c_str());
    const float label_y = std::clamp(
        slider_maximum.y + Scale(2.0f), slider_minimum.y,
        child_minimum.y + child_size.y - padding - maximum_label_size.y);
    const ImU32 label_color = ImGui::GetColorU32(
        ToImVec4(disabled ? palette.text_disabled : palette.text_secondary));
    draw_list->AddText(ImVec2(slider_minimum.x, label_y), label_color,
                       minimum_label.c_str());
    draw_list->AddText(
        ImVec2(slider_minimum.x + editor_width - maximum_label_size.x, label_y),
        label_color, maximum_label.c_str());
    draw_list->AddCircle(center, radius,
                         ImGui::GetColorU32(ToImVec4(palette.border_strong)),
                         48, Scale(1.0f));
    for (const double degrees : EvenlySpacedRotationAngles(count)) {
      const double angle =
          (degrees - 90.0) * std::numbers::pi_v<double> / 180.0;
      const ImVec2 outer(center.x + std::cos(angle) * (radius - Scale(2.0f)),
                         center.y + std::sin(angle) * (radius - Scale(2.0f)));
      const ImVec2 inner(center.x + std::cos(angle) * (radius - Scale(14.0f)),
                         center.y + std::sin(angle) * (radius - Scale(14.0f)));
      draw_list->AddLine(inner, outer,
                         ImGui::GetColorU32(ToImVec4(palette.focus)),
                         Scale(2.0f));
    }
    draw_list->AddCircleFilled(
        center, Scale(2.5f),
        ImGui::GetColorU32(
            ToImVec4(disabled ? palette.text_disabled : palette.text_primary)));
  }
  ImGui::EndChild();
  if (disabled) {
    ImGui::PopStyleColor(3);
  }
  ImGui::PopStyleColor(2);
  ImGui::PopID();

  RotationCompassResult result;
  static_cast<InteractionResult &>(result) = interaction;
  result.changed = changed && !disabled;
  result.committed = committed && !disabled;
  result.count = ClampRotationCount(count);
  ImGui::PopFont();
  return result;
}

} // namespace fancy_ui
