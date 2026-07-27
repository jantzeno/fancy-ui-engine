#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace fancy_ui::steppenface {

enum class UiIconSize : std::uint8_t {
  Small16 = 16,
  Rail24 = 24,
};

struct UiIconAssetSpec {
  std::string_view semantic_id;
  UiIconSize size = UiIconSize::Small16;
  std::string_view filename;
};

[[nodiscard]] constexpr int LogicalPixels(const UiIconSize size) {
  return static_cast<int>(size);
}

[[nodiscard]] std::span<const std::string_view> RequiredUiFontFiles();
[[nodiscard]] std::span<const UiIconAssetSpec> UiIconAssets();

} // namespace fancy_ui::steppenface
