#pragma once

#include "fancy_ui/shell/activity_rail.hpp"
#include "fancy_ui/shell/application_bar.hpp"
#include "fancy_ui/shell/context_toolbar.hpp"
#include "fancy_ui/shell/explorer.hpp"
#include "fancy_ui/shell/inspector.hpp"
#include "fancy_ui/shell/operation_regions.hpp"
#include "fancy_ui/shell/status_bar.hpp"
#include "fancy_ui/shell/workspace.hpp"

namespace fancy_ui::shell {

struct ApplicationShellState {
  bool explorer_visible = true;
  bool inspector_visible = true;
  bool operation_tray_visible = false;
  float explorer_width = 256.0f;
  float inspector_width = 320.0f;
  float operation_tray_height = 160.0f;
};

struct ApplicationShellSpec {
  ApplicationBarSpec application_bar;
  ContextToolbarSpec context_toolbar;
  ActivityRailSpec activity_rail;
  ExplorerSpec explorer;
  WorkspaceSpec workspace;
  InspectorSpec inspector;
  OperationTraySpec operation_tray;
  OperationStripSpec operation_strip;
  StatusBarSpec status_bar;
};

struct ApplicationShellResult {
  ApplicationShellState state;
  bool layout_changed = false;
};

/**
 * Draws the canonical nine-region shell in the current Dear ImGui window.
 *
 * The caller owns persistence. Pass the stored state into this function and
 * save the returned state when layout_changed is true.
 */
[[nodiscard]] ApplicationShellResult
Application(const ApplicationShellSpec &spec,
            const ApplicationShellState &state);

} // namespace fancy_ui::shell
