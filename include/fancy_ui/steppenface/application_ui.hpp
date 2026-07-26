#pragma once

#include "fancy_ui/steppenface/application_view.hpp"
#include "fancy_ui/steppenface/session_state.hpp"
#include "fancy_ui/steppenface/surface_host.hpp"
#include "fancy_ui/steppenface/ui_intent.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fancy_ui::steppenface {

struct AssetLoadReport {
  bool used_fallback_font = false;
  std::vector<std::string> messages;

  [[nodiscard]] bool ok() const { return messages.empty(); }
};

enum class BackendRequest {
  RebuildFontAtlas,
};

struct FrameResult {
  std::vector<UiIntent> product_intents;
  std::vector<BackendRequest> backend_requests;
  bool navigation_changed = false;
  bool layout_changed = false;
};

class ApplicationUi {
public:
  ApplicationUi();
  ~ApplicationUi();

  ApplicationUi(const ApplicationUi &) = delete;
  ApplicationUi &operator=(const ApplicationUi &) = delete;
  ApplicationUi(ApplicationUi &&) noexcept;
  ApplicationUi &operator=(ApplicationUi &&) noexcept;

  [[nodiscard]] AssetLoadReport
  Initialize(const std::filesystem::path &asset_root, float dpi_scale = 1.0f);
  [[nodiscard]] const SessionState &session() const;
  void SetSession(SessionState session);
  [[nodiscard]] FrameResult Draw(const ApplicationView &view,
                                 const SurfaceBindings &surfaces);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace fancy_ui::steppenface
