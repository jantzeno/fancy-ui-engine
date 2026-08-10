#include "fancy_ui/components/grain_direction_gizmo.hpp"

#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <numbers>
#include <string>

namespace fancy_ui {

namespace {

constexpr float kViewWidth = 1200.0f;
constexpr float kViewHeight = 860.0f;
constexpr ImVec2 kViewCenter{600.0f, 620.0f};
constexpr float kOuterRadius = 500.0f;
constexpr float kSpokeInnerRadius = 48.0f;
constexpr float kSpokeOuterRadius = 365.0f;
constexpr float kLabelRadius = 405.0f;
constexpr float kTickMinorRadius = 472.0f;
constexpr float kTickMidRadius = 462.0f;
constexpr float kTickMajorRadius = 446.0f;
constexpr float kNeedleLineInnerRadius = 58.0f;
constexpr float kNeedleLineOuterRadius = 355.0f;
constexpr float kNeedleHeadBaseRadius = 337.0f;
constexpr float kNeedleHeadTipRadius = 373.0f;
constexpr float kNeedleHeadHalfWidth = 16.0f;
constexpr float kNeedleLabelRadius = 315.0f;
constexpr float kHubOuterRadius = 46.0f;
constexpr float kHubInnerRadius = 20.0f;
constexpr float kHubDotRadius = 9.0f;
constexpr float kScaleFontSize = 32.0f;
constexpr float kCenterAngleFontSize = 40.0f;
constexpr float kReadoutCenterY = 705.0f;
constexpr float kReadoutWidth = 236.0f;
constexpr float kReadoutHeight = 68.0f;
constexpr float kReadoutRadius = 8.0f;
constexpr float kReadoutFontSize = 54.0f;
constexpr float kLegendCenterY = 790.0f;
constexpr float kLegendFontSize = 32.0f;

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

ImVec2 PointAtAngle(const ImVec2 center, const float radius,
                    const double degrees) {
  const double radians = degrees * std::numbers::pi / 180.0;
  return ImVec2(center.x - radius * static_cast<float>(std::cos(radians)),
                center.y - radius * static_cast<float>(std::sin(radians)));
}

GrainSnapZone HitTestZone(const ImVec2 point, const ImVec2 center,
                          const float view_scale) {
  const float toward_left = center.x - point.x;
  const float upward = center.y - point.y;
  if (upward < 0.0f) {
    return GrainSnapZone::None;
  }
  const float distance = std::hypot(toward_left, upward) / view_scale;
  if (distance <= kHubOuterRadius || distance > kOuterRadius) {
    return GrainSnapZone::None;
  }
  if (distance <= kSpokeOuterRadius) {
    return GrainSnapZone::Spokes;
  }
  if (distance < kTickMajorRadius) {
    return GrainSnapZone::Labels;
  }
  return GrainSnapZone::Ticks;
}

double AngleFromPointer(const ImVec2 point, const ImVec2 center,
                        const GrainSnapZone zone) {
  const double toward_left = center.x - point.x;
  const double upward = center.y - point.y;
  const double raw = std::atan2(upward, toward_left) * 180.0 / std::numbers::pi;
  return SnapGrainAngle(raw, zone);
}

void DrawDashedLine(ImDrawList &draw_list, const ImVec2 start, const ImVec2 end,
                    const ImU32 color, const float thickness,
                    const float view_scale) {
  const ImVec2 delta(end.x - start.x, end.y - start.y);
  const float length = std::hypot(delta.x, delta.y);
  if (length <= 0.0f) {
    return;
  }
  const ImVec2 direction(delta.x / length, delta.y / length);
  const float dash = std::max(Scale(3.0f), 16.0f * view_scale);
  const float gap = std::max(Scale(2.0f), 12.0f * view_scale);
  for (float offset = 0.0f; offset < length; offset += dash + gap) {
    const float finish = std::min(offset + dash, length);
    draw_list.AddLine(
        ImVec2(start.x + direction.x * offset, start.y + direction.y * offset),
        ImVec2(start.x + direction.x * finish, start.y + direction.y * finish),
        color, thickness);
  }
}

ColorRgba WithAlpha(ColorRgba color, const float alpha) {
  color.alpha *= alpha;
  return color;
}

ImU32 Packed(const ColorRgba color) {
  return ImGui::GetColorU32(ToImVec4(color));
}

float Stroke(const float design_width, const float view_scale) {
  return std::max(Scale(0.5f), design_width * view_scale);
}

ImFont *ResolveFont(const FontHandle handle) {
  return handle ? reinterpret_cast<ImFont *>(handle.value) : ImGui::GetFont();
}

std::string DirectionLabel(const GrainDirectionValue value) {
  const std::string_view kind =
      value.kind == GrainDirectionKind::Bed ? "BED" : "PART";
  const double degrees = ClampGrainDisplayAngle(value.degrees);
  return std::format("{} {}°", kind,
                     std::floor(degrees) == degrees
                         ? std::format("{:.0f}", degrees)
                         : std::format("{:.2f}", degrees));
}

void DrawCenteredText(ImDrawList &draw_list, ImFont *font, const float size,
                      const ImVec2 center, const ImU32 color,
                      const std::string &text) {
  const ImVec2 measured = font->CalcTextSizeA(
      size, std::numeric_limits<float>::max(), 0.0f, text.c_str());
  draw_list.AddText(
      font, size,
      ImVec2(center.x - measured.x * 0.5f, center.y - measured.y * 0.5f), color,
      text.c_str());
}

void DrawTextAtBaseline(ImDrawList &draw_list, ImFont *font, const float size,
                        const ImVec2 baseline, const float anchor,
                        const ImU32 color, const std::string &text) {
  const ImVec2 measured = font->CalcTextSizeA(
      size, std::numeric_limits<float>::max(), 0.0f, text.c_str());
  const ImFontBaked *baked = font->GetFontBaked(size);
  draw_list.AddText(
      font, size,
      ImVec2(baseline.x - measured.x * anchor, baseline.y - baked->Ascent),
      color, text.c_str());
}

void DrawOutlinedTextAtBaseline(ImDrawList &draw_list, ImFont *font,
                                const float size, const ImVec2 baseline,
                                const float anchor, const ImU32 outline,
                                const ImU32 color, const std::string &text,
                                const float view_scale) {
  const ImVec2 measured = font->CalcTextSizeA(
      size, std::numeric_limits<float>::max(), 0.0f, text.c_str());
  const ImFontBaked *baked = font->GetFontBaked(size);
  const ImVec2 position(baseline.x - measured.x * anchor,
                        baseline.y - baked->Ascent);
  const float halo = std::max(Scale(1.0f), 4.0f * view_scale);
  draw_list.AddText(font, size, ImVec2(position.x - halo, position.y), outline,
                    text.c_str());
  draw_list.AddText(font, size, ImVec2(position.x + halo, position.y), outline,
                    text.c_str());
  draw_list.AddText(font, size, ImVec2(position.x, position.y - halo), outline,
                    text.c_str());
  draw_list.AddText(font, size, ImVec2(position.x, position.y + halo), outline,
                    text.c_str());
  draw_list.AddText(font, size, ImVec2(position.x - halo, position.y - halo),
                    outline, text.c_str());
  draw_list.AddText(font, size, ImVec2(position.x + halo, position.y - halo),
                    outline, text.c_str());
  draw_list.AddText(font, size, ImVec2(position.x - halo, position.y + halo),
                    outline, text.c_str());
  draw_list.AddText(font, size, ImVec2(position.x + halo, position.y + halo),
                    outline, text.c_str());
  draw_list.AddText(font, size, position, color, text.c_str());
}

void DrawNeedle(ImDrawList &draw_list, const ImVec2 center,
                const GrainDirectionValue direction, const bool selected,
                const bool captured, const bool paired, const float view_scale,
                ImFont *label_font, const ColorRgba line_color,
                const ColorRgba label_color, const ColorRgba halo_color) {
  const double angle = ClampGrainDisplayAngle(direction.degrees);
  const ImU32 packed = Packed(line_color);
  const float thickness =
      Stroke(selected ? (captured ? 9.0f : 7.0f) : 5.0f, view_scale);
  const ImVec2 line_start =
      PointAtAngle(center, kNeedleLineInnerRadius * view_scale, angle);
  const ImVec2 line_end =
      PointAtAngle(center, kNeedleLineOuterRadius * view_scale, angle);
  if (selected) {
    draw_list.AddLine(line_start, line_end, packed, thickness);
  } else {
    DrawDashedLine(draw_list, line_start, line_end, packed, thickness,
                   view_scale);
  }

  const ImVec2 tip =
      PointAtAngle(center, kNeedleHeadTipRadius * view_scale, angle);
  const ImVec2 base =
      PointAtAngle(center, kNeedleHeadBaseRadius * view_scale, angle);
  const ImVec2 radial(tip.x - center.x, tip.y - center.y);
  const float radial_length = std::max(std::hypot(radial.x, radial.y), 1.0f);
  const ImVec2 perpendicular(-radial.y / radial_length,
                             radial.x / radial_length);
  const float half_width = kNeedleHeadHalfWidth * view_scale;
  const ImVec2 left(base.x + perpendicular.x * half_width,
                    base.y + perpendicular.y * half_width);
  const ImVec2 right(base.x - perpendicular.x * half_width,
                     base.y - perpendicular.y * half_width);
  if (selected) {
    draw_list.AddTriangleFilled(tip, left, right, packed);
  } else {
    draw_list.AddTriangleFilled(tip, left, right, Packed(halo_color));
    draw_list.AddTriangle(tip, left, right, packed, Stroke(4.0f, view_scale));
  }

  if (paired) {
    const ImVec2 label_point =
        PointAtAngle(center, kNeedleLabelRadius * view_scale, angle);
    const float font_size = std::max(Scale(9.0f), kScaleFontSize * view_scale);
    const std::string label = DirectionLabel(direction);
    const float anchor = angle < 75.0 ? 1.0f : angle > 105.0 ? 0.0f : 0.5f;
    DrawOutlinedTextAtBaseline(
        draw_list, label_font, font_size,
        ImVec2(label_point.x, label_point.y - 12.0f * view_scale), anchor,
        Packed(halo_color), Packed(label_color), label, view_scale);
  }
}

} // namespace

double ClampGrainDisplayAngle(const double degrees) {
  return std::isfinite(degrees) ? std::clamp(degrees, 0.0, 180.0) : 0.0;
}

double CanonicalGrainAxisAngle(const double degrees) {
  if (!std::isfinite(degrees)) {
    return 0.0;
  }
  double angle = std::fmod(degrees, 180.0);
  if (angle < 0.0) {
    angle += 180.0;
  }
  return angle;
}

double SnapGrainAngle(const double degrees, const GrainSnapZone zone) {
  const double step = zone == GrainSnapZone::Spokes   ? 10.0
                      : zone == GrainSnapZone::Labels ? 15.0
                      : zone == GrainSnapZone::Ticks  ? 1.0
                                                      : 0.0;
  const double clamped = ClampGrainDisplayAngle(degrees);
  return step == 0.0
             ? clamped
             : ClampGrainDisplayAngle(std::round(clamped / step) * step);
}

GrainDirectionGizmoResult
GrainDirectionGizmo(const GrainDirectionGizmoSpec &spec,
                    GrainDirectionGizmoState &state) {
  GrainDirectionGizmoResult result;
  const double committed_degrees = ClampGrainDisplayAngle(spec.primary.degrees);
  result.degrees = state.editing ? state.draft_degrees : committed_degrees;
  const SemanticPalette &palette = CurrentPalette();
  const Availability availability{
      .enabled = spec.availability.enabled && !spec.locked,
      .busy = spec.availability.busy,
      .reason = spec.locked
                    ? std::string_view("Unlock this grain direction to edit it")
                    : spec.availability.reason,
  };
  const bool enabled = availability.enabled && !availability.busy;
  const ImVec2 size(std::max(Scale(spec.size.x), Scale(160.0f)),
                    std::max(Scale(spec.size.y), Scale(120.0f)));
  const float view_scale = std::min(size.x / kViewWidth, size.y / kViewHeight);
  const ImVec2 inset((size.x - kViewWidth * view_scale) * 0.5f,
                     (size.y - kViewHeight * view_scale) * 0.5f);

  ImGui::PushID(detail::Owned(spec.id).c_str());
  detail::BeginAvailability(availability);
  ImGui::InvisibleButton("##grain-direction", size, ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  static_cast<InteractionResult &>(result) = interaction;
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 center(minimum.x + inset.x + kViewCenter.x * view_scale,
                      minimum.y + inset.y + kViewCenter.y * view_scale);
  const ImVec2 mouse = ImGui::GetIO().MousePos;
  const GrainSnapZone zone = HitTestZone(mouse, center, view_scale);
  const bool valid_zone = zone != GrainSnapZone::None;

  if (!enabled) {
    state = {};
    result.degrees = committed_degrees;
  } else {
    const bool escape = ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    if (escape && state.editing) {
      result.cancelled = true;
      result.degrees = state.original_degrees;
      state = {};
    } else {
      int key_direction = 0;
      if (interaction.focused &&
          (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) ||
           ImGui::IsKeyPressed(ImGuiKey_DownArrow, false))) {
        key_direction = -1;
      } else if (interaction.focused &&
                 (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false) ||
                  ImGui::IsKeyPressed(ImGuiKey_UpArrow, false))) {
        key_direction = 1;
      }
      if (key_direction != 0) {
        if (!state.keyboard_editing) {
          state.original_degrees = committed_degrees;
          state.draft_degrees =
              state.editing ? state.draft_degrees : committed_degrees;
        }
        state.editing = true;
        state.pointer_captured = false;
        state.keyboard_editing = true;
        const double step = ImGui::GetIO().KeyShift ? 1.0 : 15.0;
        state.draft_degrees = ClampGrainDisplayAngle(
            state.draft_degrees + step * static_cast<double>(key_direction));
      }

      if (!state.pointer_captured && !state.keyboard_editing &&
          interaction.hovered && valid_zone) {
        state.editing = true;
        state.original_degrees = committed_degrees;
        state.draft_degrees = AngleFromPointer(mouse, center, zone);
      }
      if (ImGui::IsItemActivated() && valid_zone) {
        state.editing = true;
        state.pointer_captured = true;
        state.keyboard_editing = false;
        state.original_degrees = committed_degrees;
        state.draft_degrees = AngleFromPointer(mouse, center, zone);
      }
      if (state.pointer_captured && valid_zone) {
        state.draft_degrees = AngleFromPointer(mouse, center, zone);
      }

      if (state.pointer_captured &&
          ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (valid_zone) {
          result.committed = true;
          result.changed = state.draft_degrees != state.original_degrees;
          result.degrees = state.draft_degrees;
        } else {
          result.cancelled = true;
          result.degrees = state.original_degrees;
        }
        state = {};
      } else if (state.editing && !state.pointer_captured &&
                 interaction.focused &&
                 ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
        result.committed = true;
        result.changed = state.draft_degrees != state.original_degrees;
        result.degrees = state.draft_degrees;
        state = {};
      } else if (state.keyboard_editing && !interaction.focused) {
        result.cancelled = true;
        result.degrees = state.original_degrees;
        state = {};
      } else if (!state.pointer_captured && !state.keyboard_editing &&
                 (!interaction.hovered || !valid_zone)) {
        state.editing = false;
      }

      if (state.editing) {
        result.degrees = state.draft_degrees;
        result.changed = state.draft_degrees != state.original_degrees;
      }
    }
  }
  detail::EndAvailability(availability, availability.reason);

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImFont *regular_font = ResolveFont(spec.regular_font);
  ImFont *medium_font = ResolveFont(spec.medium_font);
  ImFont *bold_font = ResolveFont(spec.bold_font);
  ImFont *monospace_font = ResolveFont(spec.monospace_font);
  const ColorRgba active_color =
      enabled ? (spec.selected ? palette.focus : palette.text_primary)
              : palette.text_disabled;
  const ColorRgba needle_color =
      enabled && state.editing ? palette.warning : active_color;
  const ColorRgba legend_color =
      enabled ? palette.text_primary : palette.text_disabled;
  const ImU32 primary_ink = Packed(palette.text_primary);
  const ImU32 geometry = Packed(palette.text_secondary);
  const ImU32 active = Packed(active_color);

  draw_list->PathArcTo(center, kOuterRadius * view_scale,
                       std::numbers::pi_v<float>,
                       std::numbers::pi_v<float> * 2.0f, 96);
  draw_list->PathStroke(primary_ink, 0, Stroke(2.0f, view_scale));
  draw_list->AddLine(PointAtAngle(center, kOuterRadius * view_scale, 0.0),
                     PointAtAngle(center, kOuterRadius * view_scale, 180.0),
                     primary_ink, Stroke(2.0f, view_scale));

  for (int degrees = 0; degrees <= 180; degrees += 10) {
    draw_list->AddLine(
        PointAtAngle(center, kSpokeInnerRadius * view_scale, degrees),
        PointAtAngle(center, kSpokeOuterRadius * view_scale, degrees), geometry,
        Stroke(1.2f, view_scale));
  }
  draw_list->PathArcTo(center, kSpokeOuterRadius * view_scale,
                       std::numbers::pi_v<float>,
                       std::numbers::pi_v<float> * 2.0f, 72);
  draw_list->PathStroke(Packed(WithAlpha(palette.text_secondary, 0.78f)), 0,
                        Stroke(1.7f, view_scale));

  for (int degrees = 0; degrees <= 180; ++degrees) {
    const float inner_radius = degrees % 10 == 0  ? kTickMajorRadius
                               : degrees % 5 == 0 ? kTickMidRadius
                                                  : kTickMinorRadius;
    const float thickness = degrees % 10 == 0  ? 2.3f
                            : degrees % 5 == 0 ? 1.7f
                                               : 1.2f;
    const float alpha = degrees % 10 == 0  ? 1.0f
                        : degrees % 5 == 0 ? 0.92f
                                           : 0.82f;
    draw_list->AddLine(PointAtAngle(center, inner_radius * view_scale, degrees),
                       PointAtAngle(center, kOuterRadius * view_scale, degrees),
                       Packed(WithAlpha(palette.text_primary, alpha)),
                       Stroke(thickness, view_scale));
  }

  const float scale_font = std::max(Scale(8.0f), kScaleFontSize * view_scale);
  const float center_angle_font =
      std::max(Scale(10.0f), kCenterAngleFontSize * view_scale);
  for (int standard = 0; standard <= 180; standard += 15) {
    const int mirror = 180 - standard;
    const ImVec2 point =
        PointAtAngle(center, kLabelRadius * view_scale, standard);
    if (standard == mirror) {
      DrawTextAtBaseline(*draw_list, bold_font, center_angle_font,
                         ImVec2(point.x, point.y + 10.0f * view_scale), 0.5f,
                         active, std::format("{}°", standard));
    } else {
      DrawTextAtBaseline(*draw_list, medium_font, scale_font,
                         ImVec2(point.x, point.y - 4.0f * view_scale), 0.5f,
                         active, std::format("{}°", standard));
      DrawTextAtBaseline(*draw_list, medium_font, scale_font,
                         ImVec2(point.x, point.y + 24.0f * view_scale), 0.5f,
                         primary_ink, std::format("{}°", mirror));
    }
  }

  if (spec.secondary.has_value()) {
    DrawNeedle(*draw_list, center, *spec.secondary, false, false, true,
               view_scale, bold_font, palette.text_secondary,
               palette.text_primary, palette.canvas);
  }
  GrainDirectionValue primary = spec.primary;
  primary.degrees = result.degrees;
  DrawNeedle(*draw_list, center, primary, true, state.pointer_captured,
             spec.secondary.has_value(), view_scale, bold_font, needle_color,
             active_color, palette.canvas);

  draw_list->AddCircleFilled(center, kHubOuterRadius * view_scale,
                             Packed(palette.canvas));
  draw_list->AddCircle(center, kHubOuterRadius * view_scale, primary_ink, 0,
                       Stroke(2.0f, view_scale));
  draw_list->AddCircleFilled(center, kHubInnerRadius * view_scale,
                             Packed(palette.surface_raised));
  draw_list->AddCircle(
      center, kHubInnerRadius * view_scale,
      Packed(enabled ? active_color : palette.control_disabled_border), 0,
      Stroke(1.5f, view_scale));
  draw_list->AddCircleFilled(center, kHubDotRadius * view_scale, active);

  const ImVec2 readout_center(minimum.x + inset.x + kViewCenter.x * view_scale,
                              minimum.y + inset.y +
                                  kReadoutCenterY * view_scale);
  const ImVec2 readout_half(kReadoutWidth * view_scale * 0.5f,
                            kReadoutHeight * view_scale * 0.5f);
  draw_list->AddRectFilled(ImVec2(readout_center.x - readout_half.x,
                                  readout_center.y - readout_half.y),
                           ImVec2(readout_center.x + readout_half.x,
                                  readout_center.y + readout_half.y),
                           Packed(palette.surface_raised),
                           kReadoutRadius * view_scale);
  draw_list->AddRect(ImVec2(readout_center.x - readout_half.x,
                            readout_center.y - readout_half.y),
                     ImVec2(readout_center.x + readout_half.x,
                            readout_center.y + readout_half.y),
                     Packed(palette.border_strong), kReadoutRadius * view_scale,
                     0, Stroke(2.0f, view_scale));
  DrawCenteredText(*draw_list, monospace_font,
                   std::max(Scale(12.0f), kReadoutFontSize * view_scale),
                   readout_center, active,
                   std::format("{:.2f}°", result.degrees));

  const ImVec2 legend_baseline(minimum.x + inset.x + kViewCenter.x * view_scale,
                               minimum.y + inset.y +
                                   kLegendCenterY * view_scale);
  DrawTextAtBaseline(*draw_list, regular_font,
                     std::max(Scale(8.0f), kLegendFontSize * view_scale),
                     legend_baseline, 0.5f, Packed(legend_color),
                     "10° spokes · 15° numbers · 1° ticks");

  detail::DrawFocusRing(interaction, true);
  ImGui::PopID();
  return result;
}

} // namespace fancy_ui
