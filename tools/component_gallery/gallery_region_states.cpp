#include "component_gallery.hpp"

#include "fancy_ui/fancy_ui.hpp"
#include "internal/component_internal.hpp"
#include "internal/operation_disclosure.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <format>
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

struct OperationStripMetrics {
  float copy_width = 230.0f;
  float progress_width = 136.0f;
  float progress_bar_width = 88.0f;
};

struct OperationDetailEntry {
  std::string_view label;
  std::string_view value;
  SemanticStatus status = SemanticStatus::Neutral;
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
                   {{"", "Bed-1.svg · 48 KB", SemanticStatus::Success},
                    {"", "Bed-2.svg · 31 KB", SemanticStatus::Success}}},
                  {"Counts",
                   {{"Beds", "2"}, {"Objects", "4"}, {"Paths", "205"}},
                   {}},
                  {"Warnings and log",
                   {},
                   {{"", "BED_UNUSED_EXCLUSION · Bed 2",
                     SemanticStatus::Warning},
                    {"", "Completed in 0.8 s", SemanticStatus::Success}}},
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
                   {{"Code", "GRAIN_CONFLICT", SemanticStatus::Failure},
                    {"Bed", "Bed 2", SemanticStatus::Failure}},
                   {}},
                  {"Conflicting objects",
                   {},
                   {{"", "Lettering artwork", SemanticStatus::Failure},
                    {"", "Bracket plate", SemanticStatus::Failure}}},
                  {"Recovery",
                   {},
                   {{"", "Review locked grain directions and retry.",
                     SemanticStatus::Warning}}},
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
                   {{"", "Front-housing-outline-final-repaired.svg · 1.2 MB",
                     SemanticStatus::Success},
                    {"",
                     "Rear-housing-production-ready-with-registration-"
                     "marks.svg "
                     "· 986 KB",
                     SemanticStatus::Success}}},
                  {"Warnings",
                   {},
                   {{"",
                     "BED_UNUSED_EXCLUSION · Bed 2 · Rear fixture exclusion "
                     "zone",
                     SemanticStatus::Warning}}},
                  {"Log",
                   {},
                   {{"",
                     "Completed all output writes and verified generated file "
                     "checksums.",
                     SemanticStatus::Success}}},
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
  ImGui::PushStyleColor(ImGuiCol_Text,
                        ToImVec4(CurrentPalette().text_secondary));
  if (font != nullptr) {
    ImGui::PushFont(font, CurrentLayoutMetrics().typography.body_font_height);
  }
  ImGui::TextUnformatted(title.data(), title.data() + title.size());
  if (font != nullptr) {
    ImGui::PopFont();
  }
  ImGui::PopStyleColor();
  ImGui::SetCursorPos(ImVec2(start.x, start.y + Scale(36.0f)));
  ImGui::Separator();
  ImGui::SetCursorPosY(start.y + Scale(37.0f));
}

void DrawOperationCardHeading(const std::string_view title, ImFont *font) {
  const ImVec2 start = ImGui::GetCursorPos();
  ImGui::SetCursorPos(ImVec2(start.x + Scale(12.0f), start.y + Scale(8.0f)));
  ImGui::PushStyleColor(ImGuiCol_Text,
                        ToImVec4(CurrentPalette().text_secondary));
  if (font != nullptr) {
    ImGui::PushFont(font, CurrentLayoutMetrics().typography.body_font_height);
  }
  ImGui::TextUnformatted(title.data(), title.data() + title.size());
  if (font != nullptr) {
    ImGui::PopFont();
  }
  ImGui::PopStyleColor();
  ImGui::SetCursorPos(ImVec2(start.x, start.y + Scale(33.0f)));
  ImGui::Separator();
  ImGui::SetCursorPosY(start.y + Scale(kOperationStateHeadingHeight));
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
    detail::ShowTooltip(presentation.label);
  }
}

void DrawOperationCopy(const OperationSample &sample, const float width,
                       ImFont *heading_font) {
  const float height = Scale(24.0f);
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(std::max(width, Scale(24.0f)), height));
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const float gap = Scale(8.0f);
  if (heading_font != nullptr) {
    ImGui::PushFont(heading_font,
                    CurrentLayoutMetrics().typography.body_font_height);
  }
  const ImVec2 label_size = ImGui::CalcTextSize(
      sample.label.data(), sample.label.data() + sample.label.size());
  const float label_width = std::min(label_size.x, width);
  const ImVec2 label_minimum(
      minimum.x,
      minimum.y + std::floor(std::max(0.0f, (height - label_size.y) * 0.5f)));
  const ImVec2 label_maximum(minimum.x + label_width, maximum.y);
  ImGui::PushStyleColor(
      ImGuiCol_Text,
      detail::StatusColor(PresentationForPhase(sample.phase).status));
  ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), label_minimum,
                            label_maximum, label_maximum.x, sample.label.data(),
                            sample.label.data() + sample.label.size(),
                            &label_size);
  ImGui::PopStyleColor();
  if (heading_font != nullptr) {
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
          maximum.x - minimum.x) {
    detail::ShowTooltip(std::format("{} · {}", sample.label, sample.detail));
  }
}

float ActionWidth(const OperationAction &action) {
  return ImGui::CalcTextSize(action.label.data(),
                             action.label.data() + action.label.size())
             .x +
         Scale(24.0f);
}

void DrawOperationProgress(const OperationSample &sample, ImFont *mono_font,
                           const OperationStripMetrics strip_metrics) {
  const LayoutMetrics metrics = CurrentLayoutMetrics();
  const float progress_width = Scale(strip_metrics.progress_width);
  const float bar_width = Scale(strip_metrics.progress_bar_width);
  const float gap = Scale(8.0f);
  const float item_height = Scale(kOperationStripItemHeight);
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  ImGui::SetCursorScreenPos(ImVec2(
      origin.x,
      origin.y +
          std::floor((item_height - metrics.geometry.progress_height) * 0.5f)));
  ProgressBar({
      .id = "operation-progress",
      .label =
          sample.indeterminate ? "Operation in progress" : "Operation progress",
      .value = sample.indeterminate ? std::nullopt : sample.progress,
      .status = SemanticStatus::Busy,
      .size = {.x = strip_metrics.progress_bar_width},
  });
  if (mono_font != nullptr) {
    ImGui::PushFont(mono_font,
                    CurrentLayoutMetrics().typography.body_font_height);
  }
  const float text_y =
      origin.y + std::floor((item_height - ImGui::GetTextLineHeight()) * 0.5f);
  ImGui::SetCursorScreenPos(ImVec2(origin.x + bar_width + gap, text_y));
  if (sample.indeterminate) {
    ImGui::TextUnformatted("Working");
  } else {
    ImGui::Text("%d%%", static_cast<int>(std::round(
                            sample.progress.value_or(0.0f) * 100.0f)));
  }
  if (mono_font != nullptr) {
    ImGui::PopFont();
  }
  ImGui::SetCursorScreenPos(origin);
  ImGui::Dummy(ImVec2(progress_width, item_height));
}

void DrawOperationStrip(detail::UiAssetAtlas &assets,
                        const OperationSample &sample,
                        OperationPresentationState &state,
                        const OperationStripMetrics strip_metrics = {}) {
  const PhasePresentation presentation = PresentationForPhase(sample.phase);
  const float strip_height = Scale(kOperationStripHeight);
  const float item_height = Scale(kOperationStripItemHeight);
  const float gap = Scale(8.0f);
  const float padding = Scale(8.0f);
  const bool has_details = !sample.sections.empty();
  if (assets.body_font() != nullptr) {
    ImGui::PushFont(assets.body_font(),
                    CurrentLayoutMetrics().typography.body_font_height);
  }
  float actions_width = 0.0f;
  for (const OperationAction &action : sample.actions) {
    actions_width += ActionWidth(action);
  }
  if (sample.actions.size() > 1) {
    actions_width += gap * static_cast<float>(sample.actions.size() - 1);
  }
  const float progress_width =
      (sample.progress.has_value() || sample.indeterminate)
          ? Scale(strip_metrics.progress_width)
          : 0.0f;

  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        detail::StatusBackground(presentation.status));
  ImGui::PushStyleColor(ImGuiCol_Border, ToImVec4(CurrentPalette().border));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(padding, Scale(3.0f)));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, 0.0f));
  if (ImGui::BeginChild("##operation-strip", ImVec2(0.0f, strip_height),
                        ImGuiChildFlags_AlwaysUseWindowPadding,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    const ImVec2 window_minimum = ImGui::GetWindowPos();
    ImGui::GetWindowDrawList()->AddLine(
        window_minimum,
        ImVec2(window_minimum.x + ImGui::GetWindowWidth(), window_minimum.y),
        ImGui::GetColorU32(ToImVec4(CurrentPalette().border)));
    const float row_y = ImGui::GetCursorScreenPos().y;
    const auto align_to_row = [row_y](const float height) {
      const ImVec2 cursor = ImGui::GetCursorScreenPos();
      ImGui::SetCursorScreenPos(ImVec2(
          cursor.x,
          row_y +
              std::floor((Scale(kOperationStripItemHeight) - height) * 0.5f)));
    };
    if (has_details) {
      if (detail::DrawOperationDisclosure(assets, state.expanded)) {
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
    const float progress_gap = progress_width > 0.0f ? gap : 0.0f;
    const float actions_gap = !sample.actions.empty() ? gap : 0.0f;
    const float copy_width = std::max(
        Scale(48.0f), std::min(Scale(strip_metrics.copy_width),
                               available - progress_width - progress_gap -
                                   actions_width - actions_gap));
    DrawOperationCopy(sample, copy_width, assets.heading_font());

    if (progress_width > 0.0f) {
      ImGui::SameLine();
      align_to_row(item_height);
      DrawOperationProgress(sample, assets.mono_font(), strip_metrics);
    }
    for (std::size_t index = 0; index < sample.actions.size(); ++index) {
      const OperationAction &action = sample.actions[index];
      if (index == 0) {
        const float actions_x = window_minimum.x + ImGui::GetWindowWidth() -
                                padding - actions_width;
        ImGui::SetCursorScreenPos(ImVec2(actions_x, row_y));
      } else {
        ImGui::SameLine();
      }
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
  if (assets.body_font() != nullptr) {
    ImGui::PopFont();
  }
}

void DrawTrayResizeHandle(OperationPresentationState &state) {
  const ResizeHandleResult result = ResizeHandle({
      .id = "tray-resize",
      .value = state.tray_height,
      .minimum = kOperationTrayMinimumHeight,
      .maximum = kOperationTrayMaximumHeight,
      .keyboard_step = 8.0f,
      .reset_value = kOperationTrayDefaultHeight,
      .direction = ResizeDirection::Vertical,
      .tooltip =
          "Drag or use Up and Down arrows to resize; double-click to reset",
  });
  if (result.changed) {
    state.tray_height = result.value;
  }
}

void DrawDetailEntry(const OperationDetailEntry &entry, const bool row_layout) {
  const bool toned = entry.status != SemanticStatus::Neutral;
  if (row_layout && toned) {
    ImGui::TextColored(detail::StatusColor(entry.status), "%.*s",
                       static_cast<int>(entry.label.size()),
                       entry.label.data());
  } else if (row_layout) {
    ImGui::TextUnformatted(entry.label.data(),
                           entry.label.data() + entry.label.size());
  }
  const ImVec2 text_size = ImGui::CalcTextSize(
      entry.value.data(), entry.value.data() + entry.value.size());
  if (row_layout) {
    ImGui::SameLine();
    const float cell_end =
        ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(
        std::max(ImGui::GetCursorPosX(), cell_end - text_size.x));
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_primary));
  } else if (toned) {
    ImGui::PushStyleColor(ImGuiCol_Text, detail::StatusColor(entry.status));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_secondary));
  }
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  ImGui::Dummy(
      ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight()));
  const ImVec2 maximum = ImGui::GetItemRectMax();
  ImGui::RenderTextEllipsis(
      ImGui::GetWindowDrawList(), minimum, maximum, maximum.x,
      entry.value.data(), entry.value.data() + entry.value.size(), &text_size);
  ImGui::PopStyleColor();
  if (ImGui::IsItemHovered()) {
    detail::ShowTooltip(entry.value);
  }
}

void DrawOperationTray(const OperationSample &sample,
                       OperationPresentationState &state, ImFont *heading_font,
                       ImFont *mono_font) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        ToImVec4(CurrentPalette().surface_muted));
  ImGui::PushStyleColor(ImGuiCol_Border,
                        ToImVec4(CurrentPalette().border_strong));
  ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(CurrentPalette().text_primary));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale(16.0f), 0.0f));
  const float tray_height = Scale(state.tray_height);
  if (ImGui::BeginChild("##operation-tray", ImVec2(0.0f, tray_height),
                        ImGuiChildFlags_AlwaysUseWindowPadding,
                        ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    const ImVec2 window_minimum = ImGui::GetWindowPos();
    ImGui::GetWindowDrawList()->AddLine(
        window_minimum,
        ImVec2(window_minimum.x + ImGui::GetWindowWidth(), window_minimum.y),
        ImGui::GetColorU32(ToImVec4(CurrentPalette().border_strong)));
    DrawTrayResizeHandle(state);
    ImGui::SetCursorPosY(Scale(16.0f));
    const int column_count =
        std::clamp(static_cast<int>(sample.sections.size()), 1, 3);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(Scale(8.0f), 0.0f));
    if (ImGui::BeginTable("##tray-sections", column_count,
                          ImGuiTableFlags_SizingStretchSame)) {
      for (const OperationDetailSection &section : sample.sections) {
        ImGui::TableNextColumn();
        ImGui::PushID(section.title.data());
        if (heading_font != nullptr) {
          ImGui::PushFont(heading_font,
                          CurrentLayoutMetrics().typography.body_font_height);
        }
        ImGui::TextUnformatted(section.title.data(),
                               section.title.data() + section.title.size());
        if (heading_font != nullptr) {
          ImGui::PopFont();
        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(4.0f));
        if (mono_font != nullptr) {
          ImGui::PushFont(mono_font,
                          CurrentLayoutMetrics().typography.body_font_height);
        }
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ToImVec4(CurrentPalette().text_secondary));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                            ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
        for (const OperationDetailEntry &row : section.rows) {
          const ImVec2 row_minimum = ImGui::GetCursorScreenPos();
          const float row_width = ImGui::GetContentRegionAvail().x;
          DrawDetailEntry(row, true);
          const float line_y = ImGui::GetItemRectMax().y;
          ImGui::GetWindowDrawList()->AddLine(
              ImVec2(row_minimum.x, line_y),
              ImVec2(row_minimum.x + row_width, line_y),
              ImGui::GetColorU32(ToImVec4(CurrentPalette().border)));
        }
        for (const OperationDetailEntry &line : section.lines) {
          DrawDetailEntry(line, false);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        if (mono_font != nullptr) {
          ImGui::PopFont();
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
    DrawOperationCardHeading(sample.title, assets.heading_font());
    if (state.expanded && !sample.sections.empty()) {
      DrawOperationTray(sample, state, assets.heading_font(),
                        assets.mono_font());
    }
    DrawOperationStrip(assets, sample, state);
    if (!state.feedback.empty()) {
      detail::DrawSecondaryText(state.feedback);
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

float CenteredContentY(const float minimum_y, const float height,
                       const float content_height) {
  return std::floor(minimum_y + std::max(0.0f, height - content_height) * 0.5f);
}

void DrawContainedFocusRing(const InteractionResult &interaction,
                            const ImVec2 minimum, const ImVec2 maximum,
                            const bool force_focus) {
  if (!force_focus && (!interaction.focused || !ImGui::GetIO().NavVisible)) {
    return;
  }
  const float inset = Scale(2.0f);
  ImGui::GetWindowDrawList()->AddRect(
      ImVec2(minimum.x + inset, minimum.y + inset),
      ImVec2(maximum.x - inset, maximum.y - inset),
      ImGui::GetColorU32(ToImVec4(CurrentPalette().focus)), Scale(3.0f),
      ImDrawFlags_RoundCornersAll, Scale(2.0f));
}

void DrawFact(const std::string_view label, const std::string_view value,
              const float width) {
  const float height = Scale(kStatusBarHeight);
  const float gap = Scale(kStatusFactLabelGap);
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(std::max(0.0f, width), height));
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const std::string label_text = std::format("{}:", std::string(label));
  const ImVec2 label_size = ImGui::CalcTextSize(label_text.c_str());
  const ImVec2 value_size =
      ImGui::CalcTextSize(value.data(), value.data() + value.size());
  const float text_y =
      CenteredContentY(minimum.y, height, std::max(label_size.y, value_size.y));
  ImGui::GetWindowDrawList()->AddText(
      ImVec2(minimum.x, text_y),
      ImGui::GetColorU32(ToImVec4(CurrentPalette().text_secondary)),
      label_text.c_str());
  const ImVec2 value_minimum(minimum.x + label_size.x + gap, text_y);
  if (value_minimum.x < maximum.x) {
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ToImVec4(CurrentPalette().text_primary));
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), value_minimum,
                              maximum, maximum.x, value.data(),
                              value.data() + value.size(), &value_size);
    ImGui::PopStyleColor();
  }
  if (ImGui::IsItemHovered() && label_size.x + gap + value_size.x > width) {
    detail::ShowTooltip(std::format("{}: {}", label, value));
  }
}

StatusBarResult DrawStatusZoomTrigger(detail::UiAssetAtlas &assets,
                                      StatusZoomPresentationState &zoom,
                                      const bool force_focus = false) {
  StatusBarResult result;
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
    const ColorRgba fill = interaction.active ? CurrentPalette().control_pressed
                                              : CurrentPalette().control_hover;
    const float visual_inset_y = Scale(2.0f);
    const ImVec2 visual_minimum(result.zoom_minimum.x,
                                result.zoom_minimum.y + visual_inset_y);
    const ImVec2 visual_maximum(result.zoom_maximum.x,
                                result.zoom_maximum.y - visual_inset_y);
    draw_list->AddRectFilled(visual_minimum, visual_maximum,
                             ImGui::GetColorU32(ToImVec4(fill)), Scale(3.0f));
    draw_list->AddRect(
        visual_minimum, visual_maximum,
        ImGui::GetColorU32(ToImVec4(CurrentPalette().border_strong)),
        Scale(3.0f), ImDrawFlags_RoundCornersAll, Scale(1.0f));
  }
  const std::string percent = std::format("{:.0f}%", std::round(zoom.percent));
  const ImVec2 text_size = ImGui::CalcTextSize(percent.c_str());
  const float icon_size = Scale(16.0f);
  const float content_width = text_size.x + Scale(2.0f) + icon_size;
  const float content_x = result.zoom_maximum.x - Scale(4.0f) - content_width;
  const float content_y =
      CenteredContentY(result.zoom_minimum.y, target.y, text_size.y);
  draw_list->AddText(
      ImVec2(content_x, content_y),
      ImGui::GetColorU32(ToImVec4(CurrentPalette().text_primary)),
      percent.c_str());
  const float icon_y =
      CenteredContentY(result.zoom_minimum.y, target.y, icon_size);
  static_cast<void>(assets.DrawIcon(
      "triangle-down", steppenface::UiIconSize::Small16,
      {.minimum = {.x = content_x + text_size.x + Scale(2.0f), .y = icon_y},
       .maximum = {.x = content_x + text_size.x + Scale(2.0f) + icon_size,
                   .y = icon_y + icon_size}},
      CurrentPalette().text_primary));
  DrawContainedFocusRing(interaction, result.zoom_minimum, result.zoom_maximum,
                         force_focus);
  if (interaction.hovered ||
      (interaction.focused && ImGui::GetIO().NavVisible)) {
    detail::ShowTooltip("Canvas zoom controls");
  }
  ImGui::PopID();
  return result;
}

StatusBarResult DrawStatusBar(detail::UiAssetAtlas &assets,
                              const StatusSample &sample,
                              StatusZoomPresentationState &zoom,
                              const bool force_focus = false,
                              const bool narrow = false) {
  if (assets.body_font() != nullptr) {
    ImGui::PushFont(assets.body_font(),
                    CurrentLayoutMetrics().typography.body_font_height);
  }
  StatusBarResult result;
  const float bar_height = Scale(kStatusBarHeight);
  const float gap = Scale(narrow ? 8.0f : kStatusFactGroupGap);
  const float cell_padding = Scale(kStatusFactCellPadding);
  const float file_width = FactWidth("File", sample.file);
  const float tool_width = FactWidth("Tool", sample.tool);
  float right_width =
      FactWidth("Grid", sample.grid) + gap + FactWidth("Snap", sample.snap);
  if (sample.workspace == StatusWorkspace::Canvas) {
    right_width += gap + Scale(68.0f);
  } else {
    right_width += gap + FactWidth("Orbit", sample.orbit) + gap +
                   FactWidth("View", sample.view);
  }

  ImGui::PushStyleColor(ImGuiCol_ChildBg,
                        ToImVec4(CurrentPalette().application_surface));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(Scale(narrow ? 4.0f : 8.0f), 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, 0.0f));
  if (ImGui::BeginChild("##status-bar", ImVec2(0.0f, bar_height),
                        ImGuiChildFlags_AlwaysUseWindowPadding,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    const ImVec2 bar_minimum = ImGui::GetWindowPos();
    const ImVec2 bar_size = ImGui::GetWindowSize();
    const ImVec2 bar_maximum(bar_minimum.x + bar_size.x,
                             bar_minimum.y + bar_size.y);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(cell_padding, 0.0f));
    if (ImGui::BeginTable(
            "##status-facts", 5,
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoClip |
                ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoSavedSettings,
            ImVec2(0.0f, bar_height))) {
      ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthFixed,
                              file_width);
      ImGui::TableSetupColumn("Tool", ImGuiTableColumnFlags_WidthFixed,
                              tool_width);
      ImGui::TableSetupColumn("Scope",
                              narrow ? ImGuiTableColumnFlags_WidthFixed
                                     : ImGuiTableColumnFlags_WidthStretch,
                              narrow ? Scale(104.0f) : 1.0f);
      ImGui::TableSetupColumn("Selection",
                              narrow ? ImGuiTableColumnFlags_WidthFixed
                                     : ImGuiTableColumnFlags_WidthStretch,
                              narrow ? Scale(104.0f) : 1.0f);
      ImGui::TableSetupColumn("View", ImGuiTableColumnFlags_WidthFixed,
                              right_width);
      ImGui::TableNextRow(ImGuiTableRowFlags_None, bar_height);
      ImGui::TableSetColumnIndex(0);
      DrawFact("File", sample.file,
               ImGui::GetContentRegionAvail().x + cell_padding * 2.0f);
      ImGui::TableSetColumnIndex(1);
      DrawFact("Tool", sample.tool,
               ImGui::GetContentRegionAvail().x + cell_padding * 2.0f);
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
        result = DrawStatusZoomTrigger(assets, zoom, force_focus);
      } else {
        ImGui::SameLine(0.0f, gap);
        DrawFact("Orbit", sample.orbit, FactWidth("Orbit", sample.orbit));
        ImGui::SameLine(0.0f, gap);
        DrawFact("View", sample.view, FactWidth("View", sample.view));
      }
      ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    ImGui::GetWindowDrawList()->AddLine(
        bar_minimum, ImVec2(bar_maximum.x, bar_minimum.y),
        ImGui::GetColorU32(ToImVec4(CurrentPalette().border)));
  }
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
  if (assets.body_font() != nullptr) {
    ImGui::PopFont();
  }
  return result;
}

ImVec2 ZoomPanelSize(const StatusSample &sample) {
  const float width =
      std::min(Scale(kStatusZoomPanelWidth), ImGui::GetContentRegionAvail().x);
  return ImVec2(width, Scale(sample.can_fit_selection ? 138.0f : 171.0f));
}

void DrawZoomPanel(const StatusSample &sample,
                   StatusZoomPresentationState &zoom, const ImVec2 panel_size,
                   const ImVec2 panel_position, ImVec2 &panel_minimum,
                   ImVec2 &panel_maximum, ImFont *mono_font) {
  const float width = panel_size.x;
  const float height = panel_size.y;
  ImGui::SetCursorScreenPos(panel_position);
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
    ImGui::BeginGroup();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(Scale(4.0f), Scale(kStatusZoomCommandSpacing)));
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    command("zoom-fit", "Zoom to Fit", sample.can_fit,
            "No spatial content is available to fit.");
    command("zoom-selection", "Zoom to Selection", sample.can_fit_selection,
            sample.selection_disabled_reason);
    command("zoom-100", "Zoom 100%", true, {});
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();
    const float label_right =
        ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    detail::DrawSecondaryText("Zoom");
    ImGui::SameLine();
    const std::string percent =
        std::format("{:.0f}%", std::round(zoom.percent));
    if (mono_font != nullptr) {
      ImGui::PushFont(mono_font,
                      CurrentLayoutMetrics().typography.body_font_height);
    }
    ImGui::SetCursorPosX(label_right - ImGui::CalcTextSize(percent.c_str()).x);
    ImGui::TextUnformatted(percent.c_str());
    if (mono_font != nullptr) {
      ImGui::PopFont();
    }
    float slider_position =
        fancy_ui::StatusZoomSliderPositionFromPercent(zoom.percent);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(ImGui::GetStyle().FramePadding.x, 0.0f));
    if (detail::DrawSliderFloat("##zoom-slider", slider_position, 0.0f, 100.0f,
                                "%.0f", {}, false, false,
                                ImGuiSliderFlags_AlwaysClamp)) {
      zoom.percent =
          fancy_ui::StatusZoomPercentFromSliderPosition(slider_position);
      zoom.feedback = "Zoom adjusted";
    }
    ImGui::PopStyleVar();
    detail::DrawFocusRing(detail::CaptureInteraction(), true);
    const float scale_right =
        ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    detail::DrawSecondaryText("10%");
    ImGui::SameLine();
    const char *maximum_label = "1600%";
    ImGui::SetCursorPosX(scale_right - ImGui::CalcTextSize(maximum_label).x);
    detail::DrawSecondaryText(maximum_label);
    if (!sample.can_fit_selection) {
      ImGui::Separator();
      detail::DrawSecondaryTextWrapped(sample.selection_disabled_reason);
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
  ImVec2 zoom_panel_size = has_zoom ? ZoomPanelSize(sample) : ImVec2{};
  const float card_height = Scale(
      sample_index == 1 || sample_index == 2 ? 192.0f : sample.card_height);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImVec4(CurrentPalette().surface));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  if (ImGui::BeginChild(
          "##status-card", ImVec2(0.0f, card_height), ImGuiChildFlags_Borders,
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
    const ImVec2 card_minimum = ImGui::GetWindowPos();
    const ImVec2 card_maximum(card_minimum.x + ImGui::GetWindowWidth(),
                              card_minimum.y + ImGui::GetWindowHeight());
    DrawStateCardHeading(sample.title, assets.heading_font());
    const float border = ImGui::GetStyle().ChildBorderSize;
    const float operation_height =
        sample_index == 6 ? Scale(kOperationStripHeight) : 0.0f;
    ImGui::SetCursorScreenPos(ImVec2(
        card_minimum.x + border,
        card_maximum.y - border - Scale(kStatusBarHeight) - operation_height));
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
      DrawOperationStrip(assets, background, state,
                         {.copy_width = 200.0f,
                          .progress_width = 132.0f,
                          .progress_bar_width = 84.0f});
    }
    const StatusBarResult bar = DrawStatusBar(assets, sample, zoom);
    if (bar.zoom_activated) {
      zoom.open = !zoom.open;
    }
    ImVec2 panel_minimum{};
    ImVec2 panel_maximum{};
    if (has_zoom && zoom.open) {
      const float inset = Scale(4.0f);
      zoom_panel_size.y =
          std::min(zoom_panel_size.y, card_height - inset * 2.0f);
      const float minimum_x = card_minimum.x + inset;
      const float maximum_x =
          std::max(minimum_x, card_maximum.x - inset - zoom_panel_size.x);
      const float panel_x = std::clamp(bar.zoom_maximum.x - zoom_panel_size.x,
                                       minimum_x, maximum_x);
      const float panel_y = bar.zoom_minimum.y - inset - zoom_panel_size.y;
      DrawZoomPanel(sample, zoom, zoom_panel_size, ImVec2(panel_x, panel_y),
                    panel_minimum, panel_maximum, assets.mono_font());
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
  detail::DrawSecondaryText(
      "Disclosure, progress, runtime actions, diagnostics, overflow, and "
      "160–240 px resize behavior.");
  ImGui::Spacing();
  if (ImGui::BeginChild("##operation-states-scroll", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    const int columns =
        ImGui::GetContentRegionAvail().x / CurrentUiScale() < 1080.0f ? 1 : 2;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,
                        ImVec2(Scale(6.0f), Scale(6.0f)));
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
    ImGui::PopStyleVar();
  }
  ImGui::EndChild();
}

void DrawStatusBarStateGallery(detail::UiAssetAtlas &assets,
                               GalleryState &state) {
  detail::DrawSecondaryText(
      "Document, tool, editing context, typed selection, view facts, Canvas "
      "zoom, overflow, and operation independence.");
  ImGui::Spacing();
  if (ImGui::BeginChild("##status-states-scroll", ImVec2(0.0f, 0.0f), false,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
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
          DrawStateCardHeading(samples[index].title, assets.heading_font());
          const float margin = Scale(12.0f);
          ImGui::SetCursorPosX(margin);
          const float narrow_width = std::min(
              Scale(760.0f), ImGui::GetContentRegionAvail().x - margin);
          ImGui::PushStyleColor(ImGuiCol_ChildBg,
                                ToImVec4(CurrentPalette().application_surface));
          ImGui::PushStyleColor(ImGuiCol_Border,
                                ToImVec4(CurrentPalette().border_strong));
          if (ImGui::BeginChild(
                  "##narrow-status", ImVec2(narrow_width, Scale(26.0f)),
                  ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
            static_cast<void>(DrawStatusBar(assets, samples[3],
                                            state.status_zoom_states[index],
                                            false, true));
          }
          ImGui::EndChild();
          ImGui::PopStyleColor(2);
          ImGui::SetCursorPos(
              ImVec2(margin, Scale(kGalleryStateHeadingHeight + 38.0f)));
          ImGui::PushID("focus-sample");
          if (assets.body_font() != nullptr) {
            ImGui::PushFont(assets.body_font(),
                            CurrentLayoutMetrics().typography.body_font_height);
          }
          static_cast<void>(DrawStatusZoomTrigger(
              assets, state.status_zoom_states[index], true));
          if (assets.body_font() != nullptr) {
            ImGui::PopFont();
          }
          ImGui::PopID();
          ImGui::SameLine(0.0f, Scale(8.0f));
          if (assets.body_font() != nullptr) {
            ImGui::PushFont(assets.body_font(),
                            CurrentLayoutMetrics().typography.body_font_height *
                                0.9f);
          }
          detail::DrawSecondaryText(
              "24 px target · 16/20 text · visible focus");
          if (assets.body_font() != nullptr) {
            ImGui::PopFont();
          }
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

void DrawShellOperationTray(detail::UiAssetAtlas &assets, GalleryState &state) {
  ImGui::PushID("shell-operation-tray");
  ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
  DrawOperationTray(OperationSamples()[1], state.shell.operation,
                    assets.heading_font(), assets.mono_font());
  ImGui::PopID();
}

void DrawShellOperationStrip(detail::UiAssetAtlas &assets,
                             GalleryState &state) {
  ImGui::PushID("shell-operation-strip");
  ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
  DrawOperationStrip(assets, OperationSamples()[1], state.shell.operation);
  ImGui::PopID();
}

void DrawShellStatusBar(detail::UiAssetAtlas &assets, GalleryState &state) {
  ImGui::PushID("shell-status-bar");
  ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
  const StatusBarResult result =
      DrawStatusBar(assets, StatusSamples()[0], state.status_zoom_states[0]);
  if (result.zoom_activated) {
    state.status_zoom_states[0].open = !state.status_zoom_states[0].open;
  }
  ImGui::PopID();
}

} // namespace fancy_ui::gallery
