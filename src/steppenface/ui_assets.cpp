#include "fancy_ui/steppenface/ui_assets.hpp"

#include <array>

namespace fancy_ui::steppenface {

std::span<const std::string_view> RequiredUiFontFiles() {
  static constexpr std::array kFonts{
      std::string_view{"NotoSans-Regular.ttf"},
      std::string_view{"NotoSans-Bold.ttf"},
      std::string_view{"NotoSansMono-Regular.ttf"},
  };
  return kFonts;
}

std::span<const UiIconAssetSpec> UiIconAssets() {
  static constexpr std::array kIcons{
      UiIconAssetSpec{"alert", UiIconSize::Small16, "alert-16.svg"},
      UiIconAssetSpec{"send", UiIconSize::Small16, "arrow-right-16.svg"},
      UiIconAssetSpec{"view", UiIconSize::Small16, "camera-16.svg"},
      UiIconAssetSpec{"check", UiIconSize::Small16, "check-16.svg"},
      UiIconAssetSpec{"machines", UiIconSize::Small16, "cpu-16.svg"},
      UiIconAssetSpec{"chevron-down", UiIconSize::Small16,
                      "chevron-down-16.svg"},
      UiIconAssetSpec{"visibility", UiIconSize::Small16, "eye-16.svg"},
      UiIconAssetSpec{"visibility-off", UiIconSize::Small16,
                      "eye-closed-16.svg"},
      UiIconAssetSpec{"fit", UiIconSize::Small16, "fit-all-16.svg"},
      UiIconAssetSpec{"focus", UiIconSize::Small16, "focus-selected-16.svg"},
      UiIconAssetSpec{"settings", UiIconSize::Small16, "gear-16.svg"},
      UiIconAssetSpec{"information", UiIconSize::Small16, "info-16.svg"},
      UiIconAssetSpec{"license", UiIconSize::Small16, "key-16.svg"},
      UiIconAssetSpec{"legal", UiIconSize::Small16, "law-16.svg"},
      UiIconAssetSpec{"layout-explorer-closed", UiIconSize::Small16,
                      "layout-left-closed-16.svg"},
      UiIconAssetSpec{"layout-explorer-open", UiIconSize::Small16,
                      "layout-left-open-16.svg"},
      UiIconAssetSpec{"layout-operation-closed", UiIconSize::Small16,
                      "layout-bottom-closed-16.svg"},
      UiIconAssetSpec{"layout-operation-open", UiIconSize::Small16,
                      "layout-bottom-open-16.svg"},
      UiIconAssetSpec{"layout-inspector-closed", UiIconSize::Small16,
                      "layout-right-closed-16.svg"},
      UiIconAssetSpec{"layout-inspector-open", UiIconSize::Small16,
                      "layout-right-open-16.svg"},
      UiIconAssetSpec{"orbit-locked", UiIconSize::Small16, "lock-16.svg"},
      UiIconAssetSpec{"more", UiIconSize::Small16, "kebab-horizontal-16.svg"},
      UiIconAssetSpec{"plus", UiIconSize::Small16, "plus-16.svg"},
      UiIconAssetSpec{"appearance", UiIconSize::Small16, "paintbrush-16.svg"},
      UiIconAssetSpec{"busy", UiIconSize::Small16, "sync-16.svg"},
      UiIconAssetSpec{"success", UiIconSize::Small16, "check-circle-16.svg"},
      UiIconAssetSpec{"triangle-down", UiIconSize::Small16,
                      "triangle-down-16.svg"},
      UiIconAssetSpec{"failure", UiIconSize::Small16, "x-circle-16.svg"},
      UiIconAssetSpec{"orbit-unlocked", UiIconSize::Small16, "unlock-16.svg"},
      UiIconAssetSpec{"model", UiIconSize::Small16, "package-16.svg"},
      UiIconAssetSpec{"model", UiIconSize::Rail24, "package-24.svg"},
      UiIconAssetSpec{"bed", UiIconSize::Small16, "bed-16.svg"},
      UiIconAssetSpec{"bed", UiIconSize::Rail24, "bed-24.svg"},
      UiIconAssetSpec{"objects", UiIconSize::Small16, "stack-16.svg"},
      UiIconAssetSpec{"objects", UiIconSize::Rail24, "stack-24.svg"},
      UiIconAssetSpec{"grain", UiIconSize::Small16, "grain-direction-16.svg"},
      UiIconAssetSpec{"grain", UiIconSize::Rail24, "grain-direction-24.svg"},
      UiIconAssetSpec{"search", UiIconSize::Small16, "search-16.svg"},
      UiIconAssetSpec{"search", UiIconSize::Rail24, "search-24.svg"},
      UiIconAssetSpec{"compact", UiIconSize::Small16, "compact-16.svg"},
      UiIconAssetSpec{"compact", UiIconSize::Rail24, "compact-24.svg"},
      UiIconAssetSpec{"diagnostics", UiIconSize::Small16, "pulse-16.svg"},
      UiIconAssetSpec{"diagnostics", UiIconSize::Rail24, "pulse-24.svg"},
      UiIconAssetSpec{"settings", UiIconSize::Rail24, "gear-24.svg"},
  };
  return kIcons;
}

} // namespace fancy_ui::steppenface
