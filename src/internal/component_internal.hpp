#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/theme.hpp"

#include <imgui.h>

#include <optional>
#include <string>
#include <string_view>

namespace fancy_ui::detail {

enum class InteractionPreview {
  Rest,
  Hovered,
  Pressed,
  Focused,
};

/**
 * Holds a transient state for deterministic developer-gallery examples.
 *
 * Production components never receive this through their public specs. The
 * override is scoped to one draw call and exists only so all interaction
 * states can be reviewed in one frame.
 */
class ScopedInteractionPreview {
public:
  explicit ScopedInteractionPreview(InteractionPreview preview);
  ~ScopedInteractionPreview();

  ScopedInteractionPreview(const ScopedInteractionPreview &) = delete;
  ScopedInteractionPreview &
  operator=(const ScopedInteractionPreview &) = delete;

private:
  std::optional<InteractionPreview> previous_;
};

/** Keeps compact developer-gallery fields inline without changing panels. */
class ScopedFieldLayoutPreview {
public:
  explicit ScopedFieldLayoutPreview(float label_width);
  ~ScopedFieldLayoutPreview();

  ScopedFieldLayoutPreview(const ScopedFieldLayoutPreview &) = delete;
  ScopedFieldLayoutPreview &
  operator=(const ScopedFieldLayoutPreview &) = delete;

private:
  std::optional<float> previous_label_width_;
};

struct ControlState {
  bool disabled = false;
  bool selected = false;
  bool invalid = false;
  bool hovered = false;
  bool pressed = false;
  bool focused = false;
  bool primary = false;
  bool tertiary = false;
  bool destructive = false;
};

struct ControlColors {
  ColorRgba fill;
  ColorRgba border;
  ColorRgba text;
};

struct FieldLayout {
  bool table = false;
  bool cell_padding_pushed = false;
};

[[nodiscard]] std::string Owned(std::string_view value);
void ShowTooltip(std::string_view text);
[[nodiscard]] float ResolveButtonVerticalPadding(float requested_height,
                                                 float text_height,
                                                 float default_padding);
[[nodiscard]] FieldLayout BeginFieldLayout(std::string_view label);
void EndFieldLayout(FieldLayout layout, const Validation &validation);
void PushFieldControlState(const Availability &availability,
                           const Validation &validation);
void PopFieldControlState(const Availability &availability,
                          const Validation &validation);
void BeginAvailability(const Availability &availability);
void EndAvailability(const Availability &availability,
                     std::string_view tooltip);
[[nodiscard]] InteractionResult CaptureInteraction();
[[nodiscard]] std::optional<InteractionPreview> CurrentInteractionPreview();
[[nodiscard]] ControlColors ResolveControlColors(const ControlState &state);
void DrawFocusRing(const InteractionResult &interaction,
                   bool high_contrast_separator = false, float rounding = 4.0f);
void DrawValidationHint(const Validation &validation);
[[nodiscard]] ImVec4 StatusColor(SemanticStatus status);
[[nodiscard]] ImVec4 StatusBackground(SemanticStatus status);

} // namespace fancy_ui::detail
