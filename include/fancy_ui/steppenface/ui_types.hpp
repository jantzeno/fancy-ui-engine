#pragma once

#include "fancy_ui/component_types.hpp"
#include "fancy_ui/steppenface/command_id.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace fancy_ui::steppenface {

struct UiId {
  std::string value;

  [[nodiscard]] bool empty() const { return value.empty(); }
  [[nodiscard]] bool operator==(const UiId &) const = default;
};

struct DurationValue {
  int hours = 0;
  int minutes = 0;

  [[nodiscard]] bool operator==(const DurationValue &) const = default;
};

struct ChoiceToggleValue {
  UiId option;
  ToggleState state = ToggleState::Off;

  [[nodiscard]] bool operator==(const ChoiceToggleValue &) const = default;
};

enum class WorkspaceKind : std::uint8_t {
  Model3d,
  Canvas,
};

enum class Destination : std::uint8_t {
  Model,
  ModelBeds,
  CanvasObjects,
  CanvasBeds,
  CanvasGrain,
  Search,
  Compact,
  Diagnostics,
};

enum class SemanticTone : std::uint8_t {
  Neutral,
  Information,
  Success,
  Warning,
  Failure,
};

enum class CommandVariant : std::uint8_t {
  Normal,
  Primary,
  Tertiary,
  Destructive,
};

enum class SelectionScope : std::uint8_t {
  Canvas,
  Object,
};

enum class SelectionTool : std::uint8_t {
  Pointer,
  Rectangle,
  Oval,
};

enum class ModelCameraPreset : std::uint8_t {
  Custom,
  Front,
  Back,
  Left,
  Right,
  Top,
  Bottom,
  Isometric,
};

struct Availability {
  bool visible = true;
  bool enabled = true;
  bool busy = false;
  std::string disabled_reason;
  BackendCapability missing_capability = BackendCapability::None;
};

struct CommandView {
  UiId id;
  CommandId command = CommandId::Quit;
  std::string label;
  std::string icon;
  std::string shortcut;
  std::string tooltip;
  CommandVariant variant = CommandVariant::Normal;
  Availability availability;
};

using FieldValue =
    std::variant<bool, std::int64_t, double, std::string, UiId, ToggleState,
                 ColorRgba, DurationValue, ChoiceToggleValue, SelectionScope,
                 SelectionTool, ModelCameraPreset>;

} // namespace fancy_ui::steppenface
