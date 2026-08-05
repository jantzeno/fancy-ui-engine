#pragma once

#include "fancy_ui/steppenface/application_view.hpp"
#include "internal/component_internal.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace fancy_ui::detail {

class UiAssetAtlas;

enum class ApplicationBarHost : std::uint8_t {
  MainViewport,
  InlineRegion,
};

enum class LayoutRegion : std::uint8_t {
  Explorer,
  OperationTray,
  Inspector,
};

struct ChromeLayoutState {
  bool explorer_visible = true;
  bool operation_tray_visible = false;
  bool operation_available = false;
  bool inspector_visible = true;
};

struct ApplicationChromeCallbacks {
  std::function<void(const steppenface::CommandView &)> invoke_command;
  std::function<void(const steppenface::ControlActionView &)> commit_action;
  std::function<void(const steppenface::FieldView &)> draw_field;
  std::function<void(steppenface::WorkspaceKind)> activate_workspace;
  std::function<void(LayoutRegion)> toggle_layout;
};

/**
 * Draws the production application bar and ordered context toolbar.
 *
 * The gallery and production composer share this renderer. The host owns
 * retained state and product mutations through callbacks.
 */
class ApplicationChrome {
public:
  explicit ApplicationChrome(UiAssetAtlas &assets);
  ~ApplicationChrome();

  ApplicationChrome(const ApplicationChrome &) = delete;
  ApplicationChrome &operator=(const ApplicationChrome &) = delete;
  ApplicationChrome(ApplicationChrome &&) noexcept;
  ApplicationChrome &operator=(ApplicationChrome &&) noexcept;

  void DrawApplicationBar(
      const steppenface::ApplicationBarView &view,
      const ChromeLayoutState &layout,
      const ApplicationChromeCallbacks &callbacks,
      ApplicationBarHost host = ApplicationBarHost::MainViewport);
  void DrawWorkspaceSwitcher(const steppenface::ApplicationBarView &view,
                             const ApplicationChromeCallbacks &callbacks,
                             std::span<const InteractionPreview> previews = {},
                             float logical_segment_width = 72.0f);
  void DrawToolbarSegmented(const steppenface::ToolbarSegmentedView &view,
                            const ApplicationChromeCallbacks &callbacks,
                            std::span<const InteractionPreview> previews = {},
                            float logical_width = 0.0f);
  void DrawContextToolbar(const steppenface::ContextToolbarView &view,
                          const ApplicationChromeCallbacks &callbacks);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace fancy_ui::detail
