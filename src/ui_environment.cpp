#include "fancy_ui/ui_environment.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreText/CoreText.h>
#endif

namespace fancy_ui {

namespace {

struct DesktopTypography {
  float base_font_em;
  UiFontWeight weight;
};

bool IsUsableScale(const float value) {
  return std::isfinite(value) && value > 0.0f;
}

UiFontWeight NearestHundredWeight(const int requested) {
  const int normalized = requested <= 0 ? 400 : std::clamp(requested, 100, 900);
  const int rounded = std::clamp(((normalized + 50) / 100) * 100, 100, 900);
  return static_cast<UiFontWeight>(rounded);
}

#if defined(_WIN32)

DesktopTypography ReadDesktopTypography() {
  NONCLIENTMETRICSW metrics{};
  metrics.cbSize = sizeof(metrics);
  if (SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics),
                                 &metrics, 0, 96) != FALSE &&
      metrics.lfMessageFont.lfHeight != 0) {
    return {
        .base_font_em =
            static_cast<float>(std::abs(metrics.lfMessageFont.lfHeight)),
        .weight = NearestHundredWeight(metrics.lfMessageFont.lfWeight),
    };
  }
  return {.base_font_em = 12.0f, .weight = UiFontWeight::Regular};
}

#elif defined(__APPLE__)

DesktopTypography ReadDesktopTypography() {
  CTFontRef font =
      CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 0.0, nullptr);
  if (font == nullptr) {
    return {.base_font_em = 13.0f, .weight = UiFontWeight::Regular};
  }
  const float size = static_cast<float>(CTFontGetSize(font));
  const bool bold = (CTFontGetSymbolicTraits(font) & kCTFontBoldTrait) != 0;
  CFRelease(font);
  return {
      .base_font_em = IsUsableScale(size) ? size : 13.0f,
      .weight = bold ? UiFontWeight::Bold : UiFontWeight::Regular,
  };
}

#else

std::filesystem::path KdeGlobalsPath() {
  if (const char *config_home = std::getenv("XDG_CONFIG_HOME");
      config_home != nullptr && config_home[0] != '\0') {
    return std::filesystem::path(config_home) / "kdeglobals";
  }
  if (const char *user_home = std::getenv("HOME");
      user_home != nullptr && user_home[0] != '\0') {
    return std::filesystem::path(user_home) / ".config" / "kdeglobals";
  }
  return {};
}

bool IsKdeDesktop() {
  const char *desktop = std::getenv("XDG_CURRENT_DESKTOP");
  if (desktop == nullptr) {
    return false;
  }
  std::string value(desktop);
  std::transform(value.begin(), value.end(), value.begin(), [](char character) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(character)));
  });
  return value.find("kde") != std::string::npos ||
         value.find("plasma") != std::string::npos;
}

std::array<std::string_view, 16> SplitQtFont(const std::string_view value) {
  std::array<std::string_view, 16> fields{};
  std::size_t begin = 0;
  std::size_t field = 0;
  while (field < fields.size() && begin <= value.size()) {
    const std::size_t end = value.find(',', begin);
    fields[field++] =
        value.substr(begin, end == std::string_view::npos ? value.size() - begin
                                                          : end - begin);
    if (end == std::string_view::npos) {
      break;
    }
    begin = end + 1;
  }
  return fields;
}

template <typename Number>
bool ParseNumber(const std::string_view text, Number &value) {
  const char *begin = text.data();
  const char *end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  return result.ec == std::errc{} && result.ptr == end;
}

UiFontWeight NearestQtWeight(const int requested) {
  if (requested >= 100) {
    return NearestHundredWeight(requested);
  }
  static constexpr std::array weights{
      std::pair{0, UiFontWeight::Thin},
      std::pair{12, UiFontWeight::ExtraLight},
      std::pair{25, UiFontWeight::Light},
      std::pair{50, UiFontWeight::Regular},
      std::pair{57, UiFontWeight::Medium},
      std::pair{63, UiFontWeight::SemiBold},
      std::pair{75, UiFontWeight::Bold},
      std::pair{81, UiFontWeight::ExtraBold},
      std::pair{87, UiFontWeight::Black},
  };
  const auto nearest =
      std::min_element(weights.begin(), weights.end(),
                       [requested](const auto &left, const auto &right) {
                         return std::abs(left.first - requested) <
                                std::abs(right.first - requested);
                       });
  return nearest->second;
}

DesktopTypography ReadDesktopTypography() {
  constexpr DesktopTypography fallback{
      .base_font_em = 40.0f / 3.0f,
      .weight = UiFontWeight::Regular,
  };
  if (!IsKdeDesktop()) {
    return fallback;
  }
  std::ifstream input(KdeGlobalsPath());
  if (!input) {
    return fallback;
  }
  bool general_section = false;
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.starts_with('[') && line.ends_with(']')) {
      general_section = line == "[General]";
      continue;
    }
    if (!general_section || !line.starts_with("font=")) {
      continue;
    }
    const auto fields = SplitQtFont(std::string_view(line).substr(5));
    float points = 0.0f;
    int weight = 50;
    if (!ParseNumber(fields[1], points) || points < 4.0f || points > 72.0f ||
        !ParseNumber(fields[4], weight)) {
      return fallback;
    }
    return {
        .base_font_em = points * 96.0f / 72.0f,
        .weight = NearestQtWeight(weight),
    };
  }
  return fallback;
}

#endif

} // namespace

UiEnvironment DetectDesktopUiEnvironment(const float display_scale,
                                         const float pixel_density) {
  const DesktopTypography typography = ReadDesktopTypography();
  UiEnvironment environment{
      .base_font_em = typography.base_font_em,
      .body_weight = typography.weight,
  };
  UpdateDisplayScales(environment, display_scale, pixel_density);
  return environment;
}

void UpdateDisplayScales(UiEnvironment &environment, float display_scale,
                         float pixel_density) {
  if (!IsUsableScale(display_scale)) {
    display_scale = 1.0f;
  }
  if (!IsUsableScale(pixel_density)) {
    pixel_density = 1.0f;
  }
  const float layout_scale = display_scale / pixel_density;
  environment.layout_scale = IsUsableScale(layout_scale) ? layout_scale : 1.0f;
  environment.raster_scale = display_scale;
}

} // namespace fancy_ui
