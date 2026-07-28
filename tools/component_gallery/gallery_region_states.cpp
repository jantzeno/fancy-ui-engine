#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/component_internal.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fancy_ui::gallery {

namespace {

enum class StatusWorkspace {
  Canvas,
  Model3d,
};

struct OperationAction {
  std::string_view id;
  std::string_view label;
  ButtonVariant variant = ButtonVariant::Secondary;
};

struct OperationDetailEntry {
  std::string_view label;
  std::string_view value;
};

struct OperationDetailSection {
  std::string_view title;
  std::vector<OperationDetailEntry> rows;
  std::vector<OperationDetailEntry> lines;
};

struct OperationSample {
  std::string_view id;
  std::string_view title;
  OperationPhase phase = OperationPhase::Preview;
  std::string_view label;
  std::string_view detail;
  std::optional<float> progress;
  bool indeterminate = false;
  std::vector<OperationAction> actions;
  std::vector<OperationDetailSection> sections;
};

struct StatusSample {
  std::string_view title;
  StatusWorkspace workspace = StatusWorkspace::Canvas;
  std::string_view file = "Fixture Kit 07";
  std::string_view tool = "Pointer";
  std::string_view scope = "Frame plate";
  std::string_view selection = "1 object";
  std::string_view grid = "10 mm";
  std::string_view snap = "On";
  std::string_view orbit;
  std::string_view view;
  bool can_fit = true;
  bool can_fit_selection = true;
  std::string_view selection_disabled_reason =
      "Select a bed, object, guide, or issue to zoom to it.";
  float card_height = 64.0f;
};

struct PhasePresentation {
  SemanticStatus status = SemanticStatus::Information;
  std::string_view icon;
  std::string_view label;
};

struct IconButtonResult : InteractionResult {
  bool activated = false;
  ImVec2 minimum;
  ImVec2 maximum;
};

struct StatusBarResult {
  bool zoom_activated = false;
  bool zoom_hovered = false;
  ImVec2 zoom_minimum;
  ImVec2 zoom_maximum;
};

ImVec4 ToImVec4(const ColorRgba color) {
  return ImVec4(color.red, color.green, color.blue, color.alpha);
}

ColorRgba FromImVec4(const ImVec4 color) {
  return {
      .red = color.x,
      .green = color.y,
      .blue = color.z,
      .alpha = color.w,
  };
}

bool Contains(const ImVec2 minimum, const ImVec2 maximum, const ImVec2 point) {
  return point.x >= minimum.x && point.x <= maximum.x && point.y >= minimum.y &&
         point.y <= maximum.y;
}

void DrawSecondaryText(const std::string_view text) {
  ImGui::TextColored(ToImVec4(CurrentPalette().text_secondary), "%.*s",
                     static_cast<int>(text.size()), text.data());
}

void DrawSecondaryTextWrapped(const std::string_view text) {
  ImGui::PushStyleColor(ImGuiCol_Text,
                        ToImVec4(CurrentPalette().text_secondary));
  ImGui::TextWrapped("%.*s", static_cast<int>(text.size()), text.data());
  ImGui::PopStyleColor();
}

PhasePresentation PresentationForPhase(const OperationPhase phase) {
  switch (phase) {
  case OperationPhase::Preview:
    return {SemanticStatus::Preview, "visibility", "Preview"};
  case OperationPhase::Running:
    return {SemanticStatus::Busy, "busy", "Running"};
  case OperationPhase::Paused:
    return {SemanticStatus::Warning, "alert", "Paused"};
  case OperationPhase::Stopping:
    return {SemanticStatus::Warning, "busy", "Stopping"};
  case OperationPhase::Finalizing:
    return {SemanticStatus::Information, "busy", "Finalizing"};
  case OperationPhase::Completed:
    return {SemanticStatus::Success, "success", "Completed"};
  case OperationPhase::Failed:
    return {SemanticStatus::Failure, "failure", "Failed"};
  }
  return {SemanticStatus::Information, "information", "Status"};
}

const std::array<OperationSample, kOperationSampleCount> &OperationSamples() {
  static const std::array<OperationSample, kOperationSampleCount> samples{{
      {
          .id = "reference-preview",
          .title = "Preview · no detail tray",
          .phase = OperationPhase::Preview,
          .label = "Guide split preview",
          .detail = "2 objects · 0 skipped",
          .actions =
              {
                  {"apply", "Apply guide split", ButtonVariant::Primary},
                  {"clear", "Clear preview", ButtonVariant::Secondary},
              },
      },
      {
          .id = "reference-running",
          .title = "Running · collapsed details",
          .phase = OperationPhase::Running,
          .label = "Search running",
          .detail = "Iteration 24 · 87.4% placed",
          .progress = 0.62f,
          .actions =
              {
                  {"pause", "Pause", ButtonVariant::Primary},
                  {"stop", "Stop", ButtonVariant::Destructive},
              },
          .sections =
              {
                  {"Current best",
                   {{"Placed", "19 of 22"}, {"Utilization", "87.4%"}},
                   {}},
                  {"Runtime", {{"Iteration", "24"}, {"Elapsed", "01:14"}}, {}},
              },
      },
      {
          .id = "reference-paused",
          .title = "Paused · expanded details",
          .phase = OperationPhase::Paused,
          .label = "Compact paused",
          .detail = "Pass 7 of 12 · 90.6%",
          .progress = 0.58f,
          .actions =
              {
                  {"resume", "Resume", ButtonVariant::Primary},
                  {"stop", "Stop", ButtonVariant::Destructive},
              },
          .sections =
              {
                  {"Current best",
                   {{"Utilization", "90.6%"}, {"Improvement", "+1.2 pp"}},
                   {}},
                  {"Pass",
                   {{"Current", "7 of 12"}, {"Accepted moves", "31"}},
                   {}},
                  {"Source",
                   {{"Result", "Search 12 / Iteration 63"}, {"Objects", "21"}},
                   {}},
              },
      },
      {
          .id = "reference-stopping",
          .title = "Stopping · collapsed details",
          .phase = OperationPhase::Stopping,
          .label = "Compact stopping",
          .detail = "Finishing the current progress callback",
          .indeterminate = true,
          .sections =
              {
                  {"Stop request",
                   {{"Source", "Preserved"},
                    {"Pending", "Worker acknowledgement"}},
                   {}},
              },
      },
      {
          .id = "reference-finalizing",
          .title = "Finalizing · collapsed details",
          .phase = OperationPhase::Finalizing,
          .label = "Compact finalizing",
          .detail = "Publishing the completed result",
          .indeterminate = true,
          .sections =
              {
                  {"Result", {{"Passes", "12"}, {"Utilization", "93.0%"}}, {}},
                  {"Persistence", {}, {{"", "Writing result and provenance…"}}},
              },
      },
      {
          .id = "reference-completed",
          .title = "Completed · expanded details",
          .phase = OperationPhase::Completed,
          .label = "Export completed",
          .detail = "2 files · 79 KB",
          .actions =
              {
                  {"open-folder", "Open folder", ButtonVariant::Primary},
                  {"dismiss", "Dismiss", ButtonVariant::Secondary},
              },
          .sections =
              {
                  {"Generated files",
                   {},
                   {{"", "Bed-1.svg · 48 KB"},
                    {"", "Bed-2.svg · 31 KB"}}},
                  {"Counts",
                   {{"Beds", "2"}, {"Objects", "4"}, {"Paths", "205"}},
                   {}},
                  {"Warnings and log",
                   {},
                   {{"", "BED_UNUSED_EXCLUSION · Bed 2"},
                    {"", "Completed in 0.8 s"}}},
              },
      },
      {
          .id = "reference-failed",
          .title = "Failed · expanded details",
          .phase = OperationPhase::Failed,
          .label = "Compact failed",
          .detail = "Constraint conflict · source preserved",
          .actions =
              {
                  {"dismiss", "Dismiss", ButtonVariant::Secondary},
              },
          .sections =
              {
                  {"Failure",
                   {{"Code", "GRAIN_CONFLICT"}, {"Bed", "Bed 2"}},
                   {}},
                  {"Conflicting objects",
                   {},
                   {{"", "Lettering artwork"},
                    {"", "Bracket plate"}}},
                  {"Recovery",
                   {},
                   {{"", "Review locked grain directions and retry."}}},
              },
      },
      {
          .id = "reference-overflow",
          .title = "Long content and maximum tray height",
          .phase = OperationPhase::Completed,
          .label = "Export completed",
          .detail =
              "Front-housing-outline-final-repaired.svg and 11 more files",
          .actions =
              {
                  {"dismiss", "Dismiss", ButtonVariant::Secondary},
              },
          .sections =
              {
                  {"Generated files",
                   {},
                   {{"", "Front-housing-outline-final-repaired.svg · 1.2 MB"},
                    {"", "Rear-housing-production-ready-with-registration-"
                         "marks.svg "
                         "· 986 KB"}}},
                  {"Warnings",
                   {},
                   {{"",
                     "BED_UNUSED_EXCLUSION · Bed 2 · Rear fixture exclusion "
                     "zone"}}},
                  {"Log",
                   {},
                   {{"",
                     "Completed all output writes and verified generated file "
                     "checksums."}}},
              },
      },
  }};
  return samples;
}

const std::array<StatusSample, kStatusSampleCount> &StatusSamples() {
  static const std::array<StatusSample, kStatusSampleCount> samples{{
      {.title = "Canvas · default"},
      {.title = "Canvas · zoom open"},
      {.title = "Canvas · empty selection",
       .scope = "Canvas",
       .selection = "None",
       .can_fit = false,
       .can_fit_selection = false},
      {.title = "Canvas · long context and multiple selection",
       .scope =
           "Bed 1 / Imported assembly / Front housing outline / Analyze and "
           "Repair",
       .selection = "12 objects · 4 guides · 4 open contours"},
      {.title = "3D · exact bed view",
       .workspace = StatusWorkspace::Model3d,
       .file = "Assembly.step",
       .scope = "Bed 1",
       .selection = "1 bed",
       .grid = "25 mm",
       .orbit = "Locked",
       .view = "Top"},
      {.title = "3D · mixed and unavailable",
       .workspace = StatusWorkspace::Model3d,
       .file = "Assembly.step",
       .scope = "Assembly.step",
       .selection = "None",
       .grid = "Mixed",
       .snap = "Mixed",
       .orbit = "Unavailable",
       .view = "Unavailable"},
      {.title = "Active operation stays independent", .card_height = 96.0f},
      {.title = "Focus, scale, and narrow width", .card_height = 112.0f},
  }};
  return samples;
}

void DrawStateCardHeading(const std::string_view title, ImFont *font) {
  const ImVec2 start = ImGui::GetCursorPos();
  ImGui::SetCursorPos(ImVec2(start.x + Scale(12.0f), start.y + Scale(8.0f)));
  if (font != nullptr) {
    ImGui::PushFont(font);
  }
  ImGui::TextUnformatted(title.data(), title.data() + title.size());
  if (font != nullptr) {
    ImGui::PopFont();
  }
  ImGui::SetCursorPos(ImVec2(start.x, start.y + Scale(36.0f)));
  ImGui::Separator();
  ImGui::SetCursorPosY(start.y + Scale(37.0f));
}

IconButtonResult DrawDisclosureButton(detail::UiAssetAtlas &assets,
                                      const bool expanded) {
  const float target = Scale(24.0f);
  const float icon_size = Scale(16.0f);
  ImGui::InvisibleButton("##operation-disclosure", ImVec2(target, target),
                         ImGuiButtonFlags_EnableNav);
  IconButtonResult result;
  static_cast<InteractionResult &>(result) = detail::CaptureInteraction();
  result.activated = ImGui::IsItemActivated();
  result.minimum = ImGui::GetItemRectMin();
  result.maximum = ImGui::GetItemRectMax();
  const SemanticPalette &palette = CurrentPalette();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (result.active || result.hovered) {
    const ColorRgba fill =
        result.active ? palette.control_pressed : palette.control_hover;
    draw_list->AddRectFilled(result.minimum, result.maximum,
                             ImGui::GetColorU32(ToImVec4(fill)), Scale(3.0f));
  }
  const float icon_x =
      std::floor((result.minimum.x + result.maximum.x - icon_size) * 0.5f);
  const float icon_y =
      std::floor((result.minimum.y + result.maximum.y - icon_size) * 0.5f);
  static_cast<void>(assets.DrawIcon(
      "chevron-down", steppenface::UiIconSize::Small16,
      {.minimum = {.x = icon_x, .y = icon_y},
       .maximum = {.x = icon_x + icon_size, .y = icon_y + icon_size}},
      palette.text_primary,
      expanded ? 0.0f : -std::numbers::pi_v<float> * 0.5f));
  detail::DrawFocusRing(result);
  if (result.hovered || (result.focused && ImGui::GetIO().NavVisible)) {
    ImGui::SetTooltip("%s operation details", expanded ? "Hide" : "Show");
  }
  return result;
}

void DrawPhaseIcon(detail::UiAssetAtlas &assets,
                   const PhasePresentation &presentation) {
  const float target = Scale(16.0f);
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(target, target));
  static_cast<void>(assets.DrawIcon(
      presentation.icon, steppenface::UiIconSize::Small16,
      {.minimum = {.x = minimum.x, .y = minimum.y},
       .maximum = {.x = minimum.x + target, .y = minimum.y + target}},
      FromImVec4(detail::StatusColor(presentation.status))));
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%.*s", static_cast<int>(presentation.label.size()),
                      presentation.label.data());
  }
}

void DrawOperationCopy(const OperationSample &sample, const float width,
                       ImFont *bold_font) {
  const float height = Scale(24.0f);
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(std::max(width, Scale(24.0f)), height));
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const float gap = Scale(8.0f);
  if (bold_font != nullptr) {
    ImGui::PushFont(bold_font);
  }
  const ImVec2 label_size = ImGui::CalcTextSize(
      sample.label.data(), sample.label.data() + sample.label.size());
  const float label_width =
      std::min(label_size.x, std::max(Scale(72.0f), width * 0.45f));
  const ImVec2 label_minimum(
      minimum.x,
      minimum.y + std::floor(std::max(0.0f, (height - label_size.y) * 0.5f)));
  const ImVec2 label_maximum(minimum.x + label_width, maximum.y);
  ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), label_minimum,
                            label_maximum, label_maximum.x, sample.label.data(),
                            sample.label.data() + sample.label.size(),
                            &label_size);
  if (bold_font != nullptr) {
    ImGui::PopFont();
  }
  const float detail_x = minimum.x + label_width + gap;
  if (detail_x < maximum.x) {
    const ImVec2 detail_size = ImGui::CalcTextSize(
        sample.detail.data(), sample.detail.data() + sample.detail.size());
    const ImVec2 detail_minimum(
        detail_x, minimum.y + std::floor(std::max(
                                  0.0f, (height - detail_size.y) * 0.5f)));
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_primary));
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), detail_minimum,
                              maximum, maximum.x, sample.detail.data(),
                              sample.detail.data() + sample.detail.size(),
                              &detail_size);
    ImGui::PopStyleColor();
  }
  if (ImGui::IsItemHovered() &&
      label_size.x + gap +
              ImGui::CalcTextSize(sample.detail.data(),
                                  sample.detail.data() + sample.detail.size())
                  .x >
          width) {
    ImGui::SetTooltip("%.*s · %.*s", static_cast<int>(sample.label.size()),
                      sample.label.data(),
                      static_cast<int>(sample.detail.size()),
                      sample.detail.data());
  }
}

float ActionWidth(const OperationAction &action) {
  return ImGui::CalcTextSize(action.label.data(),
                             action.label.data() + action.label.size())
             .x +
         Scale(24.0f);
}

void DrawOperationProgress(const OperationSample &sample) {
  const float progress_width = Scale(144.0f);
  const float bar_width = Scale(88.0f);
  const float gap = Scale(8.0f);
  const float item_height = Scale(kOperationStripItemHeight);
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  ImGui::SetCursorScreenPos(ImVec2(
      origin.x, origin.y + std::floor((item_height - Scale(6.0f)) * 0.5f)));
  ProgressBar({
      .id = "operation-progress",
      .label =
          sample.indeterminate ? "Operation in progress" : "Operation progress",
      .value = sample.indeterminate ? std::nullopt : sample.progress,
      .status = sample.indeterminate
                    ? SemanticStatus::Busy
                    : PresentationForPhase(sample.phase).status,
      .size = {.x = 88.0f, .y = 6.0f},
  });
  const float text_y =
      origin.y + std::floor((item_height - ImGui::GetTextLineHeight()) * 0.5f);
  ImGui::SetCursorScreenPos(ImVec2(origin.x + bar_width + gap, text_y));
  if (sample.indeterminate) {
    ImGui::TextUnformatted("Working");
  } else {
    ImGui::Text("%d%%", static_cast<int>(std::round(
                            sample.progress.value_or(0.0f) * 100.0f)));
  }
  ImGui::SetCursorScreenPos(origin);
  ImGui::Dummy(ImVec2(progress_width, item_height));
}

void DrawOperationStrip(detail::UiAssetAtlas &assets,
                        const OperationSample &sample,
                        OperationPresentationState &state) {
  const PhasePresentation presentation = PresentationForPhase(sample.phase);
  const float strip_height = Scale(kOperationStripHeight);
  const float item_height = Scale(kOperationStripItemHeight);
  const float gap = Scale(8.0f);
  const float padding = Scale(8.0f);
  const bool has_details = !sample.sections.empty();
  float actions_width = 0.0f;
  for (const OperationAction &action : sample.actions) {
    actions_width += ActionWidth(action);
  }
  if (sample.actions.size() > 1) {
    actions_width += gap * static_cast<float>(sample.actions.size() - 1);
  }
  const float progress_width =
      (sample.progress.has_value() || sample.indeterminate) ? Scale(144.0f)
                                                            : 0.0f;

  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        detail::StatusBackground(presentation.status));
  ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(CurrentPalette().border));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(padding, Scale(3.0f)));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, 0.0f));
  if (ImGui::BeginChild("##operation-strip", ImVec2(0.0f, strip_height),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    const float row_y = ImGui::GetCursorScreenPos().y;
    const auto align_to_row = [row_y](const float height) {
      const ImVec2 cursor = ImGui::GetCursorScreenPos();
      ImGui::SetCursorScreenPos(ImVec2(
          cursor.x,
          row_y +
              std::floor((Scale(kOperationStripItemHeight) - height) * 0.5f)));
    };
    if (has_details) {
      const IconButtonResult disclosure =
          DrawDisclosureButton(assets, state.expanded);
      if (disclosure.activated) {
        state.expanded = !state.expanded;
        state.user_toggled = true;
      }
    } else {
      ImGui::Dummy(ImVec2(Scale(24.0f), Scale(24.0f)));
    }
    ImGui::SameLine();
    align_to_row(Scale(16.0f));
    DrawPhaseIcon(assets, presentation);
    ImGui::SameLine();
    align_to_row(item_height);

    const float available = ImGui::GetContentRegionAvail().x;
    const float reserved =
        actions_width + progress_width +
        (!sample.actions.empty() && progress_width > 0.0f ? gap : 0.0f) +
        (!sample.actions.empty() ? gap : 0.0f);
    const float copy_width = std::max(Scale(48.0f), available - reserved);
    DrawOperationCopy(sample, copy_width, assets.bold_font());

    if (progress_width > 0.0f) {
      ImGui::SameLine();
      align_to_row(item_height);
      DrawOperationProgress(sample);
    }
    for (const OperationAction &action : sample.actions) {
      ImGui::SameLine();
      align_to_row(item_height);
      const ButtonResult result = Button({
          .id = action.id,
          .label = action.label,
          .variant = action.variant,
          .size = {.x = ActionWidth(action) / CurrentUiScale(), .y = 24.0f},
      });
      if (result.activated) {
        state.feedback = std::format("{} activated", std::string(action.label));
      }
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);
}

void DrawTrayResizeHandle(OperationPresentationState &state) {
  const float handle_height = Scale(8.0f);
  const float handle_width = ImGui::GetContentRegionAvail().x;
  ImGui::InvisibleButton("##tray-resize", ImVec2(handle_width, handle_height),
                         ImGuiButtonFlags_EnableNav);
  const InteractionResult interaction = detail::CaptureInteraction();
  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  if (ImGui::IsItemActivated()) {
    state.resizing = true;
    state.resize_start_mouse_y = ImGui::GetIO().MousePos.y;
    state.resize_start_height = state.tray_height;
  }
  if (ImGui::IsItemActive() && state.resizing &&
      ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    const float delta =
        (state.resize_start_mouse_y - ImGui::GetIO().MousePos.y) /
        CurrentUiScale();
    state.tray_height =
        ClampOperationTrayHeight(state.resize_start_height + delta);
  }
  if (ImGui::IsItemDeactivated()) {
    state.resizing = false;
  }
  if (interaction.hovered &&
      ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    state.tray_height = kOperationTrayDefaultHeight;
  }
  if (interaction.focused) {
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
      state.tray_height = OperationTrayHeightAfterCommand(
          state.tray_height, TrayResizeCommand::Increase);
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
      state.tray_height = OperationTrayHeightAfterCommand(
          state.tray_height, TrayResizeCommand::Decrease);
    } else if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
      state.tray_height = OperationTrayHeightAfterCommand(
          state.tray_height, TrayResizeCommand::Minimum);
    } else if (ImGui::IsKeyPressed(ImGuiKey_End)) {
      state.tray_height = OperationTrayHeightAfterCommand(
          state.tray_height, TrayResizeCommand::Maximum);
    }
  }
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const ColorRgba line_color = interaction.hovered || interaction.focused
                                   ? CurrentPalette().focus
                                   : CurrentPalette().border_strong;
  const float line_width = Scale(48.0f);
  const float line_y = minimum.y + Scale(3.0f);
  draw_list->AddRectFilled(
      ImVec2((minimum.x + maximum.x - line_width) * 0.5f, line_y),
      ImVec2((minimum.x + maximum.x + line_width) * 0.5f, line_y + Scale(2.0f)),
      ImGui::GetColorU32(ToImVec4(line_color)), Scale(1.0f));
  detail::DrawFocusRing(interaction);
  if (interaction.hovered ||
      (interaction.focused && ImGui::GetIO().NavVisible)) {
    ImGui::SetTooltip(
        "Drag or use Up and Down arrows to resize; double-click to reset");
  }
}

void DrawDetailEntry(const OperationDetailEntry &entry, const bool row_layout) {
  if (row_layout) {
    ImGui::TextUnformatted(entry.label.data(),
                           entry.label.data() + entry.label.size());
    ImGui::SameLine();
    const float value_width =
        ImGui::CalcTextSize(entry.value.data(),
                            entry.value.data() + entry.value.size())
            .x;
    const float cell_end =
        ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(
        std::max(ImGui::GetCursorPosX(), cell_end - value_width));
  }
  ImGui::TextUnformatted(entry.value.data(),
                         entry.value.data() + entry.value.size());
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%.*s", static_cast<int>(entry.value.size()),
                      entry.value.data());
  }
}

void DrawOperationTray(const OperationSample &sample,
                       OperationPresentationState &state, ImFont *bold_font) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        ToImVec4(CurrentPalette().surface_muted));
  ImGui::PushStyleColor(ImGuiCol_Border,
                        ToImVec4(CurrentPalette().border_strong));
  ImGui::PushStyleColor(ImGuiCol_Text,
                        ToImVec4(CurrentPalette().text_primary));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale(8.0f), 0.0f));
  const float tray_height = Scale(state.tray_height);
  if (ImGui::BeginChild("##operation-tray", ImVec2(0.0f, tray_height),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoSavedSettings)) {
    DrawTrayResizeHandle(state);
    ImGui::Spacing();
    const int column_count =
        std::clamp(static_cast<int>(sample.sections.size()), 1, 3);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,
                        ImVec2(Scale(8.0f), Scale(4.0f)));
    if (ImGui::BeginTable("##tray-sections", column_count,
                          ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_BordersInnerV)) {
      for (const OperationDetailSection &section : sample.sections) {
        ImGui::TableNextColumn();
        ImGui::PushID(section.title.data());
        if (bold_font != nullptr) {
          ImGui::PushFont(bold_font);
        }
        ImGui::TextUnformatted(section.title.data(),
                               section.title.data() + section.title.size());
        if (bold_font != nullptr) {
          ImGui::PopFont();
        }
        ImGui::Separator();
        for (const OperationDetailEntry &row : section.rows) {
          DrawDetailEntry(row, true);
        }
        for (const OperationDetailEntry &line : section.lines) {
          DrawDetailEntry(line, false);
        }
        ImGui::PopID();
      }
      ImGui::EndTable();
    }
    ImGui::PopStyleVar();
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(3);
}

float OperationCardHeight(const OperationSample &sample,
                          const OperationPresentationState &state) {
  return ResolveOperationLayout(state.expanded, !sample.sections.empty(),
                                state.tray_height, !state.feedback.empty())
      .content_height;
}

void DrawOperationCard(detail::UiAssetAtlas &assets,
                       const OperationSample &sample,
                       OperationPresentationState &state) {
  ImGui::TableNextColumn();
  ImGui::PushID(sample.id.data());
  const float card_height = Scale(OperationCardHeight(sample, state)) +
                            ImGui::GetStyle().ChildBorderSize * 2.0f;
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(CurrentPalette().surface));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  if (ImGui::BeginChild("##operation-card", ImVec2(0.0f, card_height),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoSavedSettings)) {
    DrawStateCardHeading(sample.title, assets.bold_font());
    if (state.expanded && !sample.sections.empty()) {
      DrawOperationTray(sample, state, assets.bold_font());
    }
    DrawOperationStrip(assets, sample, state);
    if (!state.feedback.empty()) {
      DrawSecondaryText(state.feedback);
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
  ImGui::PopID();
}

float FactWidth(const std::string_view label, const std::string_view value) {
  const std::string label_text = std::format("{}:", std::string(label));
  return ImGui::CalcTextSize(label_text.c_str()).x +
         Scale(kStatusFactLabelGap) +
         ImGui::CalcTextSize(value.data(), value.data() + value.size()).x;
}

void DrawFact(const std::string_view label, const std::string_view value,
              const float width) {
  const float height = Scale(20.0f);
  const float gap = Scale(kStatusFactLabelGap);
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(std::max(0.0f, width), height));
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const std::string label_text = std::format("{}:", std::string(label));
  const ImVec2 label_size = ImGui::CalcTextSize(label_text.c_str());
  ImGui::GetWindowDrawList()->AddText(
      minimum, ImGui::GetColorU32(ToImVec4(CurrentPalette().text_secondary)),
      label_text.c_str());
  const ImVec2 value_minimum(minimum.x + label_size.x + gap, minimum.y);
  const ImVec2 value_size =
      ImGui::CalcTextSize(value.data(), value.data() + value.size());
  if (value_minimum.x < maximum.x) {
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_primary));
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), value_minimum,
                              maximum, maximum.x, value.data(),
                              value.data() + value.size(), &value_size);
    ImGui::PopStyleColor();
  }
  if (ImGui::IsItemHovered() && label_size.x + gap + value_size.x > width) {
    ImGui::SetTooltip("%.*s: %.*s", static_cast<int>(label.size()),
                      label.data(), static_cast<int>(value.size()),
                      value.data());
  }
}

StatusBarResult DrawStatusBar(detail::UiAssetAtlas &assets,
                              const StatusSample &sample,
                              StatusZoomPresentationState &zoom,
                              const bool force_focus = false) {
  StatusBarResult result;
  const float bar_height = Scale(kStatusBarHeight);
  const float gap = Scale(kStatusFactGroupGap);
  const float cell_padding = Scale(kStatusFactCellPadding);
  const auto fixed_column_width = [cell_padding](const float content_width) {
    return content_width + cell_padding * 2.0f;
  };
  const float file_width = fixed_column_width(FactWidth("File", sample.file));
  const float tool_width = fixed_column_width(FactWidth("Tool", sample.tool));
  float right_width =
      FactWidth("Grid", sample.grid) + gap + FactWidth("Snap", sample.snap);
  if (sample.workspace == StatusWorkspace::Canvas) {
    right_width += gap + Scale(68.0f);
  } else {
    right_width += gap + FactWidth("Orbit", sample.orbit) + gap +
                   FactWidth("View", sample.view);
  }
  right_width = fixed_column_width(right_width);

  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        ToImVec4(CurrentPalette().application_surface));
  ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(CurrentPalette().border));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(Scale(10.0f), Scale(1.0f)));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, 0.0f));
  if (ImGui::BeginChild(
          "##status-bar", ImVec2(0.0f, bar_height), ImGuiChildFlags_Borders,
          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,
                        ImVec2(cell_padding, Scale(1.0f)));
    if (ImGui::BeginTable("##status-facts", 5,
                          ImGuiTableFlags_SizingStretchProp |
                              ImGuiTableFlags_NoPadOuterX |
                              ImGuiTableFlags_NoSavedSettings,
                          ImVec2(0.0f, bar_height))) {
      ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthFixed,
                              file_width);
      ImGui::TableSetupColumn("Tool", ImGuiTableColumnFlags_WidthFixed,
                              tool_width);
      ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch,
                              1.1f);
      ImGui::TableSetupColumn("Selection", ImGuiTableColumnFlags_WidthStretch,
                              0.9f);
      ImGui::TableSetupColumn("View", ImGuiTableColumnFlags_WidthFixed,
                              right_width);
      ImGui::TableNextRow(ImGuiTableRowFlags_None, Scale(20.0f));
      ImGui::TableSetColumnIndex(0);
      DrawFact("File", sample.file, ImGui::GetContentRegionAvail().x);
      ImGui::TableSetColumnIndex(1);
      DrawFact("Tool", sample.tool, ImGui::GetContentRegionAvail().x);
      ImGui::TableSetColumnIndex(2);
      DrawFact("Scope", sample.scope, ImGui::GetContentRegionAvail().x);
      ImGui::TableSetColumnIndex(3);
      DrawFact("Selection", sample.selection, ImGui::GetContentRegionAvail().x);
      ImGui::TableSetColumnIndex(4);
      DrawFact("Grid", sample.grid, FactWidth("Grid", sample.grid));
      ImGui::SameLine(0.0f, gap);
      DrawFact("Snap", sample.snap, FactWidth("Snap", sample.snap));
      if (sample.workspace == StatusWorkspace::Canvas) {
        ImGui::SameLine(0.0f, gap);
        ImGui::PushID("zoom-trigger");
        if (zoom.request_focus) {
          ImGui::SetKeyboardFocusHere();
          zoom.request_focus = false;
        }
        const ImVec2 target(Scale(68.0f), Scale(24.0f));
        ImGui::InvisibleButton("##zoom", target, ImGuiButtonFlags_EnableNav);
        const InteractionResult interaction = detail::CaptureInteraction();
        result.zoom_activated = ImGui::IsItemActivated();
        result.zoom_hovered = interaction.hovered;
        result.zoom_minimum = ImGui::GetItemRectMin();
        result.zoom_maximum = ImGui::GetItemRectMax();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        if (interaction.hovered || interaction.active || zoom.open) {
          const ColorRgba fill = interaction.active
                                     ? CurrentPalette().control_pressed
                                     : CurrentPalette().control_hover;
          draw_list->AddRectFilled(result.zoom_minimum, result.zoom_maximum,
                                   ImGui::GetColorU32(ToImVec4(fill)),
                                   Scale(3.0f));
          draw_list->AddRect(
              result.zoom_minimum, result.zoom_maximum,
              ImGui::GetColorU32(ToImVec4(CurrentPalette().border_strong)),
              Scale(3.0f));
        }
        const std::string percent =
            std::format("{:.0f}%", std::round(zoom.percent));
        const ImVec2 text_size = ImGui::CalcTextSize(percent.c_str());
        const float icon_size = Scale(16.0f);
        const float content_width = text_size.x + Scale(2.0f) + icon_size;
        const float content_x =
            result.zoom_maximum.x - Scale(4.0f) - content_width;
        const float content_y = std::floor(
            (result.zoom_minimum.y + result.zoom_maximum.y - text_size.y) *
            0.5f);
        draw_list->AddText(
            ImVec2(content_x, content_y),
            ImGui::GetColorU32(ToImVec4(CurrentPalette().text_primary)),
            percent.c_str());
        static_cast<void>(assets.DrawIcon(
            "triangle-down", steppenface::UiIconSize::Small16,
            {.minimum = {.x = content_x + text_size.x + Scale(2.0f),
                         .y = result.zoom_minimum.y + Scale(4.0f)},
             .maximum = {.x = content_x + text_size.x + Scale(2.0f) + icon_size,
                         .y = result.zoom_minimum.y + Scale(4.0f) + icon_size}},
            CurrentPalette().text_primary));
        if (force_focus) {
          InteractionResult focused = interaction;
          focused.focused = true;
          detail::DrawFocusRing(focused);
        } else {
          detail::DrawFocusRing(interaction);
        }
        if (interaction.hovered ||
            (interaction.focused && ImGui::GetIO().NavVisible)) {
          ImGui::SetTooltip("Canvas zoom controls");
        }
        ImGui::PopID();
      } else {
        ImGui::SameLine(0.0f, gap);
        DrawFact("Orbit", sample.orbit, FactWidth("Orbit", sample.orbit));
        ImGui::SameLine(0.0f, gap);
        DrawFact("View", sample.view, FactWidth("View", sample.view));
      }
      ImGui::EndTable();
    }
    ImGui::PopStyleVar();
  }
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);
  return result;
}

ImVec2 ZoomPanelSize(const StatusSample &sample) {
  const float width = std::min(Scale(280.0f), ImGui::GetContentRegionAvail().x);
  const float padding = Scale(kStatusZoomPanelPadding);
  const float spacing = Scale(kStatusZoomPanelItemSpacing);
  const float command_height = Scale(kStatusZoomCommandHeight);
  const float text_height = ImGui::GetTextLineHeight();
  float height = padding * 2.0f + command_height * 3.0f + spacing * 2.0f;
  height += spacing + text_height;
  height += spacing + ImGui::GetFrameHeight();
  height += spacing + text_height;
  if (!sample.can_fit_selection) {
    const float wrap_width = std::max(Scale(64.0f), width - padding * 2.0f);
    const ImVec2 reason_size =
        ImGui::CalcTextSize(sample.selection_disabled_reason.data(),
                            sample.selection_disabled_reason.data() +
                                sample.selection_disabled_reason.size(),
                            false, wrap_width);
    height += spacing + Scale(1.0f) + spacing + reason_size.y;
  }
  height += ImGui::GetStyle().ChildBorderSize * 2.0f;
  return ImVec2(width, std::ceil(height));
}

void DrawZoomPanel(const StatusSample &sample,
                   StatusZoomPresentationState &zoom, const ImVec2 panel_size,
                   ImVec2 &panel_minimum, ImVec2 &panel_maximum) {
  const float width = panel_size.x;
  const float height = panel_size.y;
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                       ImGui::GetContentRegionAvail().x - width);
  panel_minimum = ImGui::GetCursorScreenPos();
  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        ToImVec4(CurrentPalette().surface_raised));
  ImGui::PushStyleColor(ImGuiCol_Border,
                        ToImVec4(CurrentPalette().border_strong));
  ImGui::PushStyleVar(
      ImGuiStyleVar_WindowPadding,
      ImVec2(Scale(kStatusZoomPanelPadding), Scale(kStatusZoomPanelPadding)));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(Scale(4.0f), Scale(kStatusZoomPanelItemSpacing)));
  if (ImGui::BeginChild(
          "##zoom-panel", ImVec2(width, height), ImGuiChildFlags_Borders,
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
    const auto command = [&zoom](const char *id, const char *label,
                                 const bool enabled,
                                 const std::string_view reason) {
      const ButtonResult result = Button({
          .id = id,
          .label = label,
          .variant = ButtonVariant::Tertiary,
          .availability = {.enabled = enabled, .reason = reason},
          .size = {.x = -1.0f, .y = kStatusZoomCommandHeight},
      });
      if (result.activated) {
        zoom.feedback = std::format("{} activated", label);
        if (std::string_view(id) == "zoom-100") {
          zoom.percent = 100.0f;
        }
        zoom.open = false;
        zoom.request_focus = true;
      }
    };
    command("zoom-fit", "Zoom to Fit", sample.can_fit,
            "No spatial content is available to fit.");
    command("zoom-selection", "Zoom to Selection", sample.can_fit_selection,
            sample.selection_disabled_reason);
    command("zoom-100", "Zoom 100%", true, {});
    DrawSecondaryText("Zoom");
    ImGui::SameLine();
    ImGui::Text("%.0f%%", zoom.percent);
    float slider_position = StatusZoomSliderPositionFromPercent(zoom.percent);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::SliderFloat("##zoom-slider", &slider_position, 0.0f, 100.0f, "",
                           ImGuiSliderFlags_AlwaysClamp)) {
      zoom.percent = StatusZoomPercentFromSliderPosition(slider_position);
      zoom.feedback = "Zoom adjusted";
    }
    DrawSecondaryText("10%");
    ImGui::SameLine(ImGui::GetContentRegionMax().x -
                    ImGui::CalcTextSize("1600%").x);
    DrawSecondaryText("1600%");
    if (!sample.can_fit_selection) {
      ImGui::Separator();
      DrawSecondaryTextWrapped(sample.selection_disabled_reason);
    }
  }
  ImGui::EndChild();
  panel_maximum = ImGui::GetItemRectMax();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);
}

void DrawStatusCard(detail::UiAssetAtlas &assets, const StatusSample &sample,
                    StatusZoomPresentationState &zoom,
                    const std::size_t sample_index) {
  ImGui::PushID(static_cast<int>(sample_index));
  const bool has_zoom = sample.workspace == StatusWorkspace::Canvas;
  const ImVec2 zoom_panel_size =
      has_zoom && zoom.open ? ZoomPanelSize(sample) : ImVec2{};
  float content_height = Scale(kGalleryStateHeadingHeight + kStatusBarHeight);
  if (sample_index == 6) {
    content_height += Scale(kOperationStripHeight);
  }
  content_height += zoom_panel_size.y;
  const float card_height =
      std::max(Scale(sample.card_height),
               content_height + ImGui::GetStyle().ChildBorderSize * 2.0f);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(CurrentPalette().surface));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  if (ImGui::BeginChild(
          "##status-card", ImVec2(0.0f, card_height), ImGuiChildFlags_Borders,
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
    DrawStateCardHeading(sample.title, assets.bold_font());
    ImVec2 panel_minimum{};
    ImVec2 panel_maximum{};
    if (has_zoom && zoom.open) {
      DrawZoomPanel(sample, zoom, zoom_panel_size, panel_minimum,
                    panel_maximum);
    }
    if (sample_index == 6) {
      OperationSample background{
          .id = "background-operation",
          .phase = OperationPhase::Running,
          .label = "Search running",
          .detail = "Iteration 24 · background",
          .progress = 0.62f,
          .actions =
              {
                  {"pause", "Pause", ButtonVariant::Primary},
                  {"stop", "Stop", ButtonVariant::Destructive},
              },
      };
      OperationPresentationState state;
      DrawOperationStrip(assets, background, state);
    }
    const StatusBarResult bar = DrawStatusBar(assets, sample, zoom);
    if (bar.zoom_activated) {
      zoom.open = !zoom.open;
    }
    if (zoom.open && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      zoom.open = false;
      zoom.request_focus = true;
    }
    if (zoom.open && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      const ImVec2 mouse = ImGui::GetIO().MousePos;
      if (!Contains(panel_minimum, panel_maximum, mouse) &&
          !Contains(bar.zoom_minimum, bar.zoom_maximum, mouse)) {
        zoom.open = false;
      }
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
  ImGui::PopID();
}

} // namespace

void DrawOperationStateGallery(detail::UiAssetAtlas &assets,
                               GalleryState &state) {
  DrawSecondaryText(
      "Disclosure, progress, runtime actions, diagnostics, overflow, and "
      "160–240 px resize behavior.");
  ImGui::Spacing();
  if (ImGui::BeginChild("##operation-states-scroll", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
    const int columns =
        ImGui::GetContentRegionAvail().x / CurrentUiScale() < 1080.0f ? 1 : 2;
    if (ImGui::BeginTable("##operation-state-grid", columns,
                          ImGuiTableFlags_SizingStretchSame,
                          ImVec2(0.0f, 0.0f))) {
      const auto &samples = OperationSamples();
      for (std::size_t index = 0; index < samples.size(); ++index) {
        DrawOperationCard(assets, samples[index],
                          state.operation_states[index]);
      }
      ImGui::EndTable();
    }
  }
  ImGui::EndChild();
}

void DrawStatusBarStateGallery(detail::UiAssetAtlas &assets,
                               GalleryState &state) {
  DrawSecondaryText(
      "Document, tool, editing context, typed selection, view facts, Canvas "
      "zoom, overflow, and operation independence.");
  ImGui::Spacing();
  if (ImGui::BeginChild("##status-states-scroll", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
    const auto &samples = StatusSamples();
    for (std::size_t index = 0; index < samples.size(); ++index) {
      if (index == 7) {
        ImGui::PushID("narrow-card");
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
                              ToImVec4(CurrentPalette().surface));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::BeginChild("##status-card",
                              ImVec2(0.0f, Scale(samples[index].card_height)),
                              ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_NoScrollbar)) {
          DrawStateCardHeading(samples[index].title, assets.bold_font());
          const float narrow_width =
              std::min(Scale(760.0f), ImGui::GetContentRegionAvail().x);
          ImGui::PushStyleColor(ImGuiCol_ChildBg,
                                ToImVec4(CurrentPalette().application_surface));
          if (ImGui::BeginChild("##narrow-status", ImVec2(narrow_width, 0.0f),
                                ImGuiChildFlags_Borders,
                                ImGuiWindowFlags_NoScrollbar)) {
            static_cast<void>(DrawStatusBar(
                assets, samples[3], state.status_zoom_states[index], true));
          }
          ImGui::EndChild();
          ImGui::PopStyleColor();
          ImGui::SameLine();
          DrawSecondaryText("24 px target · 16/20 text · visible focus");
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopID();
      } else {
        DrawStatusCard(assets, samples[index], state.status_zoom_states[index],
                       index);
      }
      if (index + 1 < samples.size()) {
        ImGui::Spacing();
      }
    }
  }
  ImGui::EndChild();
}

} // namespace fancy_ui::gallery
