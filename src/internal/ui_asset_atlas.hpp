#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/steppenface/ui_assets.hpp"

#include <imgui.h>

#include <filesystem>
#include <memory>
#include <string_view>

namespace fancy_ui::detail {

/**
 * Owns the one ImGui font atlas used by the application or developer gallery.
 *
 * SVG masters stay in the repository; this helper rasterizes their alpha masks
 * at the requested UI scale and installs them after the renderer creates the
 * font texture.
 */
class UiAssetAtlas {
public:
  UiAssetAtlas();
  ~UiAssetAtlas();

  UiAssetAtlas(const UiAssetAtlas &) = delete;
  UiAssetAtlas &operator=(const UiAssetAtlas &) = delete;
  UiAssetAtlas(UiAssetAtlas &&) noexcept;
  UiAssetAtlas &operator=(UiAssetAtlas &&) noexcept;

  [[nodiscard]] steppenface::AssetLoadReport
  Load(const std::filesystem::path &asset_root, float ui_scale);
  void InstallPendingIcons();

  [[nodiscard]] bool DrawIcon(std::string_view semantic_id,
                              steppenface::UiIconSize size, const Rect &bounds,
                              ColorRgba color) const;
  [[nodiscard]] IconPainter
  Painter(std::string_view semantic_id,
          steppenface::UiIconSize size = steppenface::UiIconSize::Small16);

  [[nodiscard]] ImFont *regular_font() const;
  [[nodiscard]] ImFont *bold_font() const;
  [[nodiscard]] ImFont *mono_font() const;
  [[nodiscard]] float ui_scale() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace fancy_ui::detail
