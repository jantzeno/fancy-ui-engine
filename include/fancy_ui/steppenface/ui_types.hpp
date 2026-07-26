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

struct TreeRowView {
  UiId id;
  std::string label;
  std::string secondary_label;
  std::string icon;
  int depth = 0;
  bool expanded = false;
  bool expandable = false;
  bool selected = false;
  bool visible = true;
  std::optional<ColorRgba> color;
};

using FieldValue =
    std::variant<bool, std::int64_t, double, std::string, SelectionScope,
                 SelectionTool, ModelCameraPreset>;

struct FieldView {
  UiId id;
  std::string label;
  FieldValue value;
  std::optional<UiId> target;
  std::string unit;
  std::string help;
  Availability availability;
};

struct SectionView {
  UiId id;
  std::string heading;
  bool initially_open = true;
  std::vector<FieldView> fields;
  std::vector<CommandView> commands;
};

struct StatusItemView {
  UiId id;
  std::string label;
  SemanticTone tone = SemanticTone::Neutral;
};

struct OperationView {
  UiId id;
  std::string title;
  std::string summary;
  SemanticTone tone = SemanticTone::Neutral;
  float progress = 0.0f;
  bool indeterminate = false;
  std::vector<CommandView> commands;
};

} // namespace fancy_ui::steppenface
