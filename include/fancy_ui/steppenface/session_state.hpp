#pragma once

#include "fancy_ui/steppenface/ui_types.hpp"

#include <string>
#include <unordered_map>

namespace fancy_ui::steppenface {

[[nodiscard]] constexpr WorkspaceKind
WorkspaceForDestination(const Destination destination) {
  switch (destination) {
  case Destination::Model:
  case Destination::ModelBeds:
    return WorkspaceKind::Model3d;
  case Destination::CanvasObjects:
  case Destination::CanvasBeds:
  case Destination::CanvasGrain:
  case Destination::Search:
  case Destination::Compact:
  case Destination::Diagnostics:
    return WorkspaceKind::Canvas;
  }
  return WorkspaceKind::Model3d;
}

struct SessionState {
  Destination active_destination = Destination::Model;
  Destination last_model_destination = Destination::Model;
  Destination last_canvas_destination = Destination::CanvasObjects;
  bool explorer_visible = true;
  bool inspector_visible = true;
  bool operation_tray_visible = false;
  float explorer_width = 256.0f;
  float inspector_width = 320.0f;
  float operation_tray_height = 160.0f;
  std::unordered_map<Destination, std::string> explorer_queries;
  std::unordered_map<std::string, bool> explorer_expanded_rows;
  std::unordered_map<std::string, bool> collapsed_sections;
  UiId model_grid_target{.value = "all"};

  void ActivateDestination(const Destination destination) {
    active_destination = destination;
    if (WorkspaceForDestination(destination) == WorkspaceKind::Model3d) {
      last_model_destination = destination;
    } else {
      last_canvas_destination = destination;
    }
  }

  void ActivateWorkspace(const WorkspaceKind workspace) {
    active_destination = workspace == WorkspaceKind::Model3d
                             ? last_model_destination
                             : last_canvas_destination;
  }
};

} // namespace fancy_ui::steppenface
