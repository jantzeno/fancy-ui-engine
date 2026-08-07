#include "internal/ui_asset_atlas.hpp"

#include "fancy_ui/layout_metrics.hpp"
#include "fancy_ui/theme.hpp"

#include <lunasvg.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fancy_ui::detail {

namespace {

std::string IconAtlasKey(const std::string_view semantic_id,
                         const steppenface::UiIconSize size) {
  return std::string(semantic_id) + "@" +
         std::to_string(steppenface::LogicalPixels(size));
}

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

} // namespace

class UiAssetAtlas::Impl {
public:
  struct PendingIcon {
    std::string key;
    lunasvg::Bitmap bitmap;
  };

  ImFont *regular_font = nullptr;
  ImFont *bold_font = nullptr;
  ImFont *mono_font = nullptr;
  float ui_scale = 1.0f;
  std::unordered_map<std::string, ImFontAtlasRectId> icon_rects;
  std::vector<PendingIcon> pending_icons;
};

UiAssetAtlas::UiAssetAtlas() : impl_(std::make_unique<Impl>()) {}
UiAssetAtlas::~UiAssetAtlas() = default;
UiAssetAtlas::UiAssetAtlas(UiAssetAtlas &&) noexcept = default;
UiAssetAtlas &UiAssetAtlas::operator=(UiAssetAtlas &&) noexcept = default;

steppenface::AssetLoadReport
UiAssetAtlas::Load(const std::filesystem::path &asset_root,
                   const float requested_scale) {
  steppenface::AssetLoadReport report;
  if (ImGui::GetCurrentContext() == nullptr) {
    report.used_fallback_font = true;
    report.messages.emplace_back(
        "Fancy UI asset loading requires an active ImGui context");
    return report;
  }

  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->Clear();
  impl_->ui_scale = std::clamp(requested_scale, 0.75f, 2.0f);
  const auto load_font = [&report, &asset_root](const std::string_view name,
                                                const float size) -> ImFont * {
    const std::filesystem::path path = asset_root / "fonts" / std::string(name);
    if (!std::filesystem::is_regular_file(path)) {
      report.messages.push_back("Missing UI font: " + path.string());
      return nullptr;
    }
    ImFont *font =
        ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str(), size);
    if (font == nullptr) {
      report.messages.push_back("Could not load UI font: " + path.string());
    }
    return font;
  };

  const std::span<const std::string_view> fonts =
      steppenface::RequiredUiFontFiles();
  const float body_font_height =
      ResolveLayoutMetrics(impl_->ui_scale).typography.body_font_height;
  impl_->regular_font = load_font(fonts[0], body_font_height);
  impl_->bold_font = load_font(fonts[1], 18.0f * impl_->ui_scale);
  impl_->mono_font = load_font(fonts[2], body_font_height);
  if (impl_->regular_font == nullptr) {
    impl_->regular_font = io.Fonts->AddFontDefault();
    report.used_fallback_font = true;
  }

  impl_->icon_rects.clear();
  impl_->pending_icons.clear();
  for (const steppenface::UiIconAssetSpec &asset :
       steppenface::UiIconAssets()) {
    const std::filesystem::path path =
        asset_root / "icons" / std::string(asset.filename);
    if (!std::filesystem::is_regular_file(path)) {
      report.messages.push_back("Missing UI icon: " + path.string());
      continue;
    }
    std::unique_ptr<lunasvg::Document> document =
        lunasvg::Document::loadFromFile(path.string());
    if (document == nullptr) {
      report.messages.push_back("Could not load UI icon: " + path.string());
      continue;
    }
    const int logical_pixels = steppenface::LogicalPixels(asset.size);
    const int icon_pixels = std::max(
        logical_pixels,
        static_cast<int>(std::round(logical_pixels * impl_->ui_scale)));
    lunasvg::Bitmap bitmap = document->renderToBitmap(icon_pixels, icon_pixels);
    if (bitmap.isNull()) {
      report.messages.push_back("Could not rasterize UI icon: " +
                                path.string());
      continue;
    }
    bitmap.convertToRGBA();
    impl_->pending_icons.push_back(
        {.key = IconAtlasKey(asset.semantic_id, asset.size),
         .bitmap = std::move(bitmap)});
  }

  io.FontDefault = impl_->regular_font;
  ApplyTheme(ResolvedTheme::Dark, impl_->ui_scale);
  return report;
}

void UiAssetAtlas::InstallPendingIcons() {
  ImFontAtlas *atlas = ImGui::GetIO().Fonts;
  if (impl_->pending_icons.empty() || !atlas->RendererHasTextures ||
      atlas->TexData == nullptr || atlas->TexData->Pixels == nullptr) {
    return;
  }

  for (const Impl::PendingIcon &icon : impl_->pending_icons) {
    ImFontAtlasRect rect;
    const ImFontAtlasRectId rect_id =
        atlas->AddCustomRect(icon.bitmap.width(), icon.bitmap.height(), &rect);
    if (rect_id == ImFontAtlasRectId_Invalid) {
      continue;
    }

    ImTextureData *texture = atlas->TexData;
    if (texture == nullptr || texture->Pixels == nullptr ||
        static_cast<int>(rect.x) + icon.bitmap.width() > texture->Width ||
        static_cast<int>(rect.y) + icon.bitmap.height() > texture->Height) {
      atlas->RemoveCustomRect(rect_id);
      continue;
    }

    for (int y = 0; y < icon.bitmap.height(); ++y) {
      const unsigned char *source =
          icon.bitmap.data() + y * icon.bitmap.stride();
      unsigned char *destination =
          texture->Pixels + ((static_cast<int>(rect.y) + y) * texture->Width +
                             static_cast<int>(rect.x)) *
                                texture->BytesPerPixel;
      for (int x = 0; x < icon.bitmap.width(); ++x) {
        if (texture->BytesPerPixel == 4) {
          destination[x * 4 + 0] = 255;
          destination[x * 4 + 1] = 255;
          destination[x * 4 + 2] = 255;
          destination[x * 4 + 3] = source[x * 4 + 3];
        } else {
          destination[x] = source[x * 4 + 3];
        }
      }
    }
    impl_->icon_rects.emplace(icon.key, rect_id);
  }
  impl_->pending_icons.clear();
}

bool UiAssetAtlas::DrawIcon(const std::string_view semantic_id,
                            const steppenface::UiIconSize size,
                            const Rect &bounds, const ColorRgba color,
                            const float rotation_radians) const {
  const auto found = impl_->icon_rects.find(IconAtlasKey(semantic_id, size));
  if (found == impl_->icon_rects.end()) {
    return false;
  }
  ImFontAtlasRect rect;
  if (!ImGui::GetIO().Fonts->GetCustomRect(found->second, &rect)) {
    return false;
  }
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const ImU32 tint = ImGui::GetColorU32(ToImVec4(color));
  if (std::abs(rotation_radians) < std::numeric_limits<float>::epsilon()) {
    draw_list->AddImage(ImGui::GetIO().Fonts->TexRef,
                        ImVec2(bounds.minimum.x, bounds.minimum.y),
                        ImVec2(bounds.maximum.x, bounds.maximum.y), rect.uv0,
                        rect.uv1, tint);
    return true;
  }

  const ImVec2 center((bounds.minimum.x + bounds.maximum.x) * 0.5f,
                      (bounds.minimum.y + bounds.maximum.y) * 0.5f);
  const float half_width = (bounds.maximum.x - bounds.minimum.x) * 0.5f;
  const float half_height = (bounds.maximum.y - bounds.minimum.y) * 0.5f;
  const float cosine = std::cos(rotation_radians);
  const float sine = std::sin(rotation_radians);
  const auto rotate = [center, cosine, sine](const float x, const float y) {
    return ImVec2(center.x + x * cosine - y * sine,
                  center.y + x * sine + y * cosine);
  };
  draw_list->AddImageQuad(
      ImGui::GetIO().Fonts->TexRef, rotate(-half_width, -half_height),
      rotate(half_width, -half_height), rotate(half_width, half_height),
      rotate(-half_width, half_height), rect.uv0,
      ImVec2(rect.uv1.x, rect.uv0.y), rect.uv1, ImVec2(rect.uv0.x, rect.uv1.y),
      tint);
  return true;
}

IconPainter UiAssetAtlas::Painter(const std::string_view semantic_id,
                                  const steppenface::UiIconSize size) {
  const std::string owned_id(semantic_id);
  return [this, owned_id, size](const Rect &bounds, const ColorRgba color) {
    static_cast<void>(DrawIcon(owned_id, size, bounds, color));
  };
}

ImFont *UiAssetAtlas::regular_font() const { return impl_->regular_font; }
ImFont *UiAssetAtlas::bold_font() const { return impl_->bold_font; }
ImFont *UiAssetAtlas::mono_font() const { return impl_->mono_font; }
float UiAssetAtlas::ui_scale() const { return impl_->ui_scale; }

} // namespace fancy_ui::detail
