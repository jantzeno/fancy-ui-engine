#pragma once

#include "fancy_ui/component_types.hpp"

#include <span>
#include <string_view>

namespace fancy_ui {

enum class SpatialOverlayPattern {
  Solid,
  Dashed,
  Hatched,
};

struct SpatialOverlaySpec {
  std::string_view id;
  std::span<const Vec2> points;
  std::string_view label;
  SemanticStatus status = SemanticStatus::Preview;
  SpatialOverlayPattern pattern = SpatialOverlayPattern::Solid;
  bool closed = true;
  bool selected = false;
  bool focused = false;
};

void SpatialOverlay(const SpatialOverlaySpec &spec);

} // namespace fancy_ui
