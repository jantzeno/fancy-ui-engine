#pragma once

#include "fancy_ui/component_types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace fancy_ui {

inline constexpr int kRotationCountMinimum = 1;
inline constexpr int kRotationCountMaximum = 16;

[[nodiscard]] int ClampRotationCount(int value);
[[nodiscard]] double RotationStepDegrees(int count);
[[nodiscard]] std::vector<double> EvenlySpacedRotationAngles(int count);
[[nodiscard]] std::string FormatRotationDegrees(double degrees);

struct RotationCompassSpec {
  std::string_view id;
  std::string_view label;
  std::string_view tooltip;
  int count = 4;
  bool inherited = false;
  Availability availability;
};

struct RotationCompassResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  int count = 4;
};

/**
 * Draws the effective search rotations from one count value.
 *
 * The count, step label, active ticks, and returned angles all use the same
 * derivation so callers cannot show a preview that disagrees with the value.
 */
[[nodiscard]] RotationCompassResult
RotationCompass(const RotationCompassSpec &spec);

} // namespace fancy_ui
