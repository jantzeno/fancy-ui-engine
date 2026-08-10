#pragma once

#include "fancy_ui/component_types.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

[[nodiscard]] double ClampGrainDisplayAngle(double degrees);
[[nodiscard]] double CanonicalGrainAxisAngle(double degrees);

enum class GrainSnapZone {
  None,
  Spokes,
  Labels,
  Ticks,
};

[[nodiscard]] double SnapGrainAngle(double degrees, GrainSnapZone zone);

enum class GrainDirectionKind {
  Bed,
  Part,
};

struct GrainDirectionValue {
  GrainDirectionKind kind = GrainDirectionKind::Bed;
  double degrees = 0.0;
};

struct GrainDirectionGizmoState {
  bool editing = false;
  bool pointer_captured = false;
  bool keyboard_editing = false;
  double original_degrees = 0.0;
  double draft_degrees = 0.0;
};

struct GrainDirectionGizmoSpec {
  std::string_view id;
  GrainDirectionValue primary;
  std::optional<GrainDirectionValue> secondary;
  bool selected = true;
  bool locked = false;
  Availability availability;
  FontHandle regular_font;
  FontHandle medium_font;
  FontHandle bold_font;
  FontHandle monospace_font;
  Vec2 size{480.0f, 344.0f};
};

struct GrainDirectionGizmoResult : InteractionResult {
  bool changed = false;
  bool committed = false;
  bool cancelled = false;
  double degrees = 0.0;
};

[[nodiscard]] GrainDirectionGizmoResult
GrainDirectionGizmo(const GrainDirectionGizmoSpec &spec,
                    GrainDirectionGizmoState &state);

} // namespace fancy_ui
