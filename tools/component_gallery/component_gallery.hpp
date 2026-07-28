#pragma once

#include "fancy_ui/theme.hpp"

namespace fancy_ui::detail {
class UiAssetAtlas;
}

namespace fancy_ui::gallery {

struct GalleryState {
  ResolvedTheme theme = ResolvedTheme::Dark;
  float scale = 1.0f;
};

void DrawComponentGallery(detail::UiAssetAtlas &assets, GalleryState &state);

} // namespace fancy_ui::gallery
