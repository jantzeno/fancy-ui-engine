#pragma once

#include <string_view>

namespace fancy_ui {

/**
 * Describes whether a prepared command can be invoked by a component.
 *
 * The caller calculates availability and its explanation from product state.
 * Components only render the supplied state.
 */
struct Availability {
  bool enabled = true;
  bool busy = false;
  std::string_view reason;
};

/**
 * Interaction facts common to all immediate-mode components.
 */
struct InteractionResult {
  bool hovered = false;
  bool focused = false;
  bool active = false;
};

enum class SemanticStatus {
  Neutral,
  Information,
  Success,
  Warning,
  Failure,
};

} // namespace fancy_ui
