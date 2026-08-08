#pragma once

#include "fancy_ui/ui_environment.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fancy_ui::steppenface {

struct AssetLoadReport {
  bool used_fallback_font = false;
  std::vector<std::string> messages;

  [[nodiscard]] bool ok() const { return messages.empty(); }
};

enum class UiIconSize : std::uint8_t {
  Small16 = 16,
  Rail24 = 24,
};

struct UiIconAssetSpec {
  std::string_view semantic_id;
  UiIconSize size = UiIconSize::Small16;
  std::string_view filename;
};

struct UiFontAssetSpec {
  std::string_view filename;
  UiFontWeight weight = UiFontWeight::Regular;
  bool monospace = false;
};

[[nodiscard]] constexpr int LogicalPixels(const UiIconSize size) {
  return static_cast<int>(size);
}

[[nodiscard]] std::span<const UiFontAssetSpec> RequiredUiFontAssets();
[[nodiscard]] std::span<const UiIconAssetSpec> UiIconAssets();

} // namespace fancy_ui::steppenface
