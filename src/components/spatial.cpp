#include "fancy_ui/components/spatial.hpp"

#include "fancy_ui/components/button.hpp"

namespace fancy_ui {

SpatialControlResult SpatialControl(const SpatialControlSpec &spec) {
  const ButtonResult result = Button({
      .id = spec.id,
      .label = spec.label,
      .tooltip = spec.tooltip,
      .variant =
          spec.selected ? ButtonVariant::Primary : ButtonVariant::Secondary,
      .size = ImVec2(0.0f, 32.0f),
  });
  return {.activated = result.activated};
}

} // namespace fancy_ui
