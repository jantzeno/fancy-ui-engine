#include "component_gallery.hpp"

#include "fancy_ui/steppenface/ui_assets.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct HostOptions {
  fancy_ui::gallery::GalleryState gallery;
  std::filesystem::path screenshot;
  std::string capture_state;
  std::optional<float> display_scale_override;
  std::optional<float> pixel_density_override;
  bool valid = true;
};

GLADloadproc SdlGlProcLoader() {
  return reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress);
}

std::optional<float> ParsePositiveFloat(const char *text) {
  char *end = nullptr;
  const float value = std::strtof(text, &end);
  if (end == text || *end != '\0' || !std::isfinite(value) || value <= 0.0f) {
    return std::nullopt;
  }
  return value;
}

HostOptions ParseOptions(const int argc, char **argv) {
  HostOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--theme" && index + 1 < argc) {
      const std::string_view value(argv[++index]);
      options.gallery.theme = value == "light" ? fancy_ui::ResolvedTheme::Light
                                               : fancy_ui::ResolvedTheme::Dark;
      options.gallery.settings.system_theme = options.gallery.theme;
    } else if (argument == "--scale" && index + 1 < argc) {
      const std::optional<float> scale = ParsePositiveFloat(argv[++index]);
      if (!scale.has_value()) {
        std::cerr << "Scale must be a positive finite number\n";
        options.valid = false;
      } else {
        options.display_scale_override = *scale;
      }
    } else if (argument == "--pixel-density" && index + 1 < argc) {
      const std::optional<float> density = ParsePositiveFloat(argv[++index]);
      if (!density.has_value()) {
        std::cerr << "Pixel density must be a positive finite number\n";
        options.valid = false;
      } else {
        options.pixel_density_override = *density;
      }
    } else if (argument == "--screenshot" && index + 1 < argc) {
      options.screenshot = argv[++index];
    } else if (argument == "--state" && index + 1 < argc) {
      options.capture_state = argv[++index];
    } else if (argument == "--tab" && index + 1 < argc) {
      const std::string_view value(argv[++index]);
      const std::optional<fancy_ui::gallery::GalleryTab> tab =
          fancy_ui::gallery::ParseGalleryTab(value);
      if (!tab.has_value()) {
        std::cerr << "Unknown gallery tab: " << value
                  << " (expected components, shell, panel-audits, settings, "
                     "operations, or status)\n";
        options.valid = false;
      } else {
        fancy_ui::gallery::ActivateGalleryTab(options.gallery, *tab);
      }
    }
  }
  if (!options.capture_state.empty() &&
      !fancy_ui::gallery::SeedGalleryCaptureState(options.gallery,
                                                  options.capture_state)) {
    std::cerr << "Unknown gallery capture state: " << options.capture_state
              << '\n';
    options.valid = false;
  }
  if (options.display_scale_override.has_value()) {
    options.gallery.scale = *options.display_scale_override /
                            options.pixel_density_override.value_or(1.0f);
  }
  return options;
}

int Fail(const std::string_view message) {
  std::cerr << message << ": " << SDL_GetError() << '\n';
  return 1;
}

bool WriteScreenshot(const std::filesystem::path &path, const int width,
                     const int height) {
  std::vector<unsigned char> pixels(
      static_cast<std::size_t>(width * height * 4));
  std::vector<unsigned char> flipped(pixels.size());
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  const std::size_t stride = static_cast<std::size_t>(width * 4);
  for (int row = 0; row < height; ++row) {
    const auto source =
        pixels.begin() + static_cast<std::ptrdiff_t>((height - row - 1) *
                                                     static_cast<int>(stride));
    std::copy_n(source, stride,
                flipped.begin() + static_cast<std::ptrdiff_t>(
                                      row * static_cast<int>(stride)));
  }
  return stbi_write_png(path.string().c_str(), width, height, 4, flipped.data(),
                        static_cast<int>(stride)) != 0;
}

} // namespace

int main(const int argc, char **argv) {
  HostOptions options = ParseOptions(argc, argv);
  if (!options.valid) {
    return 2;
  }
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    return Fail("SDL initialization failed");
  }

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_WindowFlags window_flags =
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (!options.screenshot.empty()) {
    window_flags |= SDL_WINDOW_HIDDEN;
  }
  int window_width = 1280;
  int window_height = 1024;
  if (!options.screenshot.empty()) {
    const fancy_ui::gallery::GalleryCaptureExtent extent =
        fancy_ui::gallery::GalleryScreenshotLogicalExtent(
            options.gallery.active_tab);
    window_width =
        static_cast<int>(std::lround(extent.width * options.gallery.scale));
    window_height =
        static_cast<int>(std::lround(extent.height * options.gallery.scale));
  }
  SDL_Window *window = SDL_CreateWindow("Fancy UI gallery", window_width,
                                        window_height, window_flags);
  if (window == nullptr) {
    SDL_Quit();
    return Fail("Window creation failed");
  }
  float display_scale = options.display_scale_override.value_or(
      SDL_GetWindowDisplayScale(window));
  float pixel_density = options.pixel_density_override.value_or(
      options.display_scale_override.has_value()
          ? 1.0f
          : SDL_GetWindowPixelDensity(window));
  fancy_ui::UiEnvironment environment =
      fancy_ui::DetectDesktopUiEnvironment(display_scale, pixel_density);
  options.gallery.scale = environment.layout_scale;
  if (!options.screenshot.empty()) {
    const fancy_ui::gallery::GalleryCaptureExtent extent =
        fancy_ui::gallery::GalleryScreenshotLogicalExtent(
            options.gallery.active_tab);
    SDL_SetWindowSize(
        window,
        static_cast<int>(std::lround(extent.width * environment.layout_scale)),
        static_cast<int>(
            std::lround(extent.height * environment.layout_scale)));
  }
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == nullptr || !SDL_GL_MakeCurrent(window, gl_context)) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return Fail("OpenGL context creation failed");
  }
  if (!gladLoadGLLoader(SdlGlProcLoader())) {
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return Fail("OpenGL loading failed");
  }
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::GetIO().IniFilename = nullptr;
  fancy_ui::detail::UiAssetAtlas assets;
  const fancy_ui::steppenface::AssetLoadReport report = assets.Load(
      std::filesystem::path(FANCY_UI_GALLERY_ASSET_ROOT), environment);
  for (const std::string &message : report.messages) {
    std::cerr << message << '\n';
  }
  if (!ImGui_ImplSDL3_InitForOpenGL(window, gl_context) ||
      !ImGui_ImplOpenGL3_Init("#version 330")) {
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return Fail("ImGui backend initialization failed");
  }

  bool running = true;
  int rendered_frames = 0;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      running = running && event.type != SDL_EVENT_QUIT;
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
          event.window.windowID == SDL_GetWindowID(window)) {
        running = false;
      }
      if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED &&
          event.window.windowID == SDL_GetWindowID(window) &&
          !options.display_scale_override.has_value()) {
        fancy_ui::UpdateDisplayScales(environment,
                                      SDL_GetWindowDisplayScale(window),
                                      options.pixel_density_override.value_or(
                                          SDL_GetWindowPixelDensity(window)));
        options.gallery.scale = environment.layout_scale;
        const fancy_ui::steppenface::AssetLoadReport update_report =
            assets.Load(std::filesystem::path(FANCY_UI_GALLERY_ASSET_ROOT),
                        environment);
        for (const std::string &message : update_report.messages) {
          std::cerr << message << '\n';
        }
      }
    }
    if (!running) {
      break;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    fancy_ui::gallery::DrawComponentGallery(assets, options.gallery);
    ImGui::Render();

    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    glViewport(0, 0, width, height);
    const fancy_ui::SemanticPalette &palette = fancy_ui::CurrentPalette();
    glClearColor(palette.application_surface.red,
                 palette.application_surface.green,
                 palette.application_surface.blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ++rendered_frames;
    if (!options.screenshot.empty() && rendered_frames >= 3) {
      if (!WriteScreenshot(options.screenshot, width, height)) {
        std::cerr << "Could not write screenshot: "
                  << options.screenshot.string() << '\n';
      }
      running = false;
    }
    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DestroyContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
