#include "internal/operation_disclosure.hpp"

#include "fancy_ui/components/operation_disclosure.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <numbers>

namespace fancy_ui::detail {

bool DrawOperationDisclosure(UiAssetAtlas &assets, const bool expanded,
                             const bool available) {
  return OperationDisclosure(
             {
                 .id = "operation-disclosure",
                 .expanded = expanded,
                 .icon =
                     [&assets, expanded](const Rect &bounds,
                                         const ColorRgba color) {
                       static_cast<void>(assets.DrawIcon(
                           "chevron-down", steppenface::UiIconSize::Small16,
                           bounds, color,
                           expanded ? 0.0f
                                    : -std::numbers::pi_v<float> * 0.5f));
                     },
                 .availability = {.enabled = available},
             })
      .changed;
}

} // namespace fancy_ui::detail
