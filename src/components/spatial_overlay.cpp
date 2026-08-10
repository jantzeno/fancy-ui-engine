#include "fancy_ui/components/spatial_overlay.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/component_internal.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace fancy_ui {

namespace {

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

void DrawDashedLine(ImDrawList &draw_list, const ImVec2 start, const ImVec2 end,
                    const ImU32 color, const float thickness) {
  const ImVec2 delta(end.x - start.x, end.y - start.y);
  const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
  if (length <= 0.0f) {
    return;
  }
  const ImVec2 direction(delta.x / length, delta.y / length);
  const float dash = Scale(6.0f);
  const float gap = Scale(4.0f);
  for (float offset = 0.0f; offset < length; offset += dash + gap) {
    const float end_offset = std::min(offset + dash, length);
    draw_list.AddLine(
        ImVec2(start.x + direction.x * offset, start.y + direction.y * offset),
        ImVec2(start.x + direction.x * end_offset,
               start.y + direction.y * end_offset),
        color, thickness);
  }
}

} // namespace

namespace detail {

std::vector<HatchSegment> HatchSegments(const std::span<const Vec2> polygon,
                                        const float spacing) {
  std::vector<HatchSegment> segments;
  if (polygon.size() < 3 || !std::isfinite(spacing) || spacing <= 0.0f) {
    return segments;
  }

  constexpr float epsilon = 0.001f;
  float minimum_sum = std::numeric_limits<float>::max();
  float maximum_sum = std::numeric_limits<float>::lowest();
  for (const Vec2 point : polygon) {
    const float sum = point.x + point.y;
    minimum_sum = std::min(minimum_sum, sum);
    maximum_sum = std::max(maximum_sum, sum);
  }
  const float first = std::floor(minimum_sum / spacing) * spacing;
  std::vector<Vec2> intersections;
  intersections.reserve(polygon.size());
  for (float line = first; line < maximum_sum; line += spacing) {
    intersections.clear();
    for (std::size_t index = 0; index < polygon.size(); ++index) {
      const Vec2 start = polygon[index];
      const Vec2 end = polygon[(index + 1) % polygon.size()];
      const float start_sum = start.x + start.y;
      const float end_sum = end.x + end.y;
      const bool crosses = (start_sum <= line && line < end_sum) ||
                           (end_sum <= line && line < start_sum);
      if (!crosses || std::abs(end_sum - start_sum) <= epsilon) {
        continue;
      }
      const float t = (line - start_sum) / (end_sum - start_sum);
      intersections.push_back({.x = start.x + (end.x - start.x) * t,
                               .y = start.y + (end.y - start.y) * t});
    }
    std::ranges::sort(intersections, {},
                      [](const Vec2 point) { return point.x - point.y; });
    intersections.erase(
        std::unique(intersections.begin(), intersections.end(),
                    [](const Vec2 left, const Vec2 right) {
                      return std::abs(left.x - right.x) <= epsilon &&
                             std::abs(left.y - right.y) <= epsilon;
                    }),
        intersections.end());
    for (std::size_t index = 1; index < intersections.size(); index += 2) {
      const Vec2 start = intersections[index - 1];
      const Vec2 end = intersections[index];
      if (std::hypot(end.x - start.x, end.y - start.y) > epsilon) {
        segments.push_back({.start = start, .end = end});
      }
    }
  }
  return segments;
}

} // namespace detail

void SpatialOverlay(const SpatialOverlaySpec &spec) {
  if (spec.points.empty()) {
    return;
  }
  std::vector<ImVec2> points;
  std::vector<Vec2> screen_points;
  points.reserve(spec.points.size());
  screen_points.reserve(spec.points.size());
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  Vec2 extent;
  for (const Vec2 point : spec.points) {
    const Vec2 screen{.x = origin.x + Scale(point.x),
                      .y = origin.y + Scale(point.y)};
    points.emplace_back(screen.x, screen.y);
    screen_points.push_back(screen);
    extent.x = std::max(extent.x, point.x);
    extent.y = std::max(extent.y, point.y);
  }
  ImGui::PushID(detail::Owned(spec.id).c_str());
  ImGui::Dummy(ImVec2(Scale(extent.x), Scale(extent.y)));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec4 color = detail::StatusColor(spec.status);
  if (spec.selected) {
    color = ToImVec4(CurrentPalette().focus);
  }
  const ImU32 stroke = ImGui::GetColorU32(color);
  const float thickness = Scale(spec.focused ? 3.0f : 2.0f);
  if (spec.closed && points.size() >= 3) {
    ImVec4 fill = color;
    fill.w = 0.08f;
    draw_list->AddConvexPolyFilled(points.data(),
                                   static_cast<int>(points.size()),
                                   ImGui::GetColorU32(fill));
  }
  const std::size_t segment_count =
      spec.closed ? points.size() : points.size() - std::size_t{1};
  for (std::size_t index = 0; index < segment_count; ++index) {
    const ImVec2 start = points[index];
    const ImVec2 end = points[(index + 1) % points.size()];
    if (spec.pattern == SpatialOverlayPattern::Solid) {
      draw_list->AddLine(start, end, stroke, thickness);
    } else {
      DrawDashedLine(*draw_list, start, end, stroke, thickness);
    }
  }
  if (spec.pattern == SpatialOverlayPattern::Hatched && spec.closed) {
    for (const detail::HatchSegment &segment :
         detail::HatchSegments(screen_points, Scale(12.0f))) {
      draw_list->AddLine(ImVec2(segment.start.x, segment.start.y),
                         ImVec2(segment.end.x, segment.end.y), stroke,
                         Scale(1.0f));
    }
  }
  if (!spec.label.empty()) {
    const std::string label = detail::Owned(spec.label);
    const ImVec2 label_position(points.front().x + Scale(6.0f),
                                points.front().y + Scale(6.0f));
    const ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
    const ImVec2 label_minimum(label_position.x - Scale(3.0f),
                               label_position.y - Scale(2.0f));
    const ImVec2 label_maximum(label_position.x + label_size.x + Scale(3.0f),
                               label_position.y + label_size.y + Scale(2.0f));
    draw_list->AddRectFilled(
        label_minimum, label_maximum,
        ImGui::GetColorU32(ToImVec4(CurrentPalette().surface_raised)),
        Scale(2.0f));
    draw_list->AddRect(label_minimum, label_maximum, stroke, Scale(2.0f));
    draw_list->AddText(label_position, stroke, label.c_str());
  }
  ImGui::PopID();
}

} // namespace fancy_ui
