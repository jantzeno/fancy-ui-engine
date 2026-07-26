#pragma once

#include "fancy_ui/steppenface/ui_types.hpp"

#include <cstdint>

namespace im2d {
struct CanvasState;
}

namespace fancy_ui::steppenface {

struct PointerState {
  Vec2 position;
  Vec2 delta;
  float wheel = 0.0f;
  bool hovered = false;
  bool primary_down = false;
  bool primary_clicked = false;
  bool primary_released = false;
  bool secondary_down = false;
  bool secondary_clicked = false;
  bool secondary_released = false;
  bool shift = false;
  bool control = false;
  bool alt = false;
};

struct ModelSurfaceRequest {
  Vec2 logical_size;
  Vec2 framebuffer_scale{1.0f, 1.0f};
  PointerState pointer;
};

struct ModelSurfaceFrame {
  TextureHandle texture;
  Vec2 logical_size;
  bool ready = false;
  bool input_captured = false;
};

class ModelSurfaceHost {
public:
  virtual ~ModelSurfaceHost() = default;
  [[nodiscard]] virtual ModelSurfaceFrame
  Render(const ModelSurfaceRequest &request) = 0;
};

class CanvasSurfaceBinding {
public:
  CanvasSurfaceBinding() = default;
  explicit CanvasSurfaceBinding(im2d::CanvasState &state) : state_(&state) {}

  [[nodiscard]] bool valid() const { return state_ != nullptr; }

private:
  friend class ApplicationUi;
  im2d::CanvasState *state_ = nullptr;
};

struct SurfaceBindings {
  ModelSurfaceHost *model = nullptr;
  CanvasSurfaceBinding *canvas = nullptr;
};

} // namespace fancy_ui::steppenface
