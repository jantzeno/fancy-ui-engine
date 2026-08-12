#include "fancy_ui/layout_metrics.hpp"

#include "fancy_ui/theme.hpp"

#include <algorithm>
#include <cmath>

namespace fancy_ui {

namespace {

constexpr float kNotoVerticalMetricsPerEm = 1362.0f / 1000.0f;
constexpr float kDefaultBodyFontEm = 40.0f / 3.0f;

constexpr LayoutMetrics kLogicalMetrics{
    .spacing =
        {
            .condensed = 1.0f,
            .space01 = 2.0f,
            .space02 = 4.0f,
            .space03 = 8.0f,
            .space04 = 12.0f,
            .space05 = 16.0f,
            .space06 = 24.0f,
            .space07 = 32.0f,
            .space08 = 40.0f,
            .space09 = 48.0f,
            .space10 = 64.0f,
            .space11 = 80.0f,
            .space12 = 96.0f,
            .space13 = 160.0f,
        },
    .geometry =
        {
            .border = 1.0f,
            .focus_ring = 2.0f,
            .control_radius = 3.0f,
            .surface_radius = 4.0f,
            .child_window_radius = 5.0f,
            .icon = 16.0f,
            .activity_icon = 24.0f,
            .progress_height = 6.0f,
            .compact_target = 24.0f,
            .control_height = 32.0f,
            .row_height = 32.0f,
            .panel_header_height = 56.0f,
        },
    .typography =
        {
            .body_font_height = kDefaultBodyFontEm * kNotoVerticalMetricsPerEm,
            .section_heading_font_height =
                kDefaultBodyFontEm * 1.125f * kNotoVerticalMetricsPerEm,
            .settings_title_font_height =
                kDefaultBodyFontEm * 1.25f * kNotoVerticalMetricsPerEm,
            .page_title_font_height =
                kDefaultBodyFontEm * 1.5f * kNotoVerticalMetricsPerEm,
        },
    .menu =
        {
            .popup_padding_horizontal = 10.0f,
            .popup_padding_vertical = 8.0f,
            .popup_width = 264.0f,
            .trigger_rounding = 4.0f,
        },
    .shell =
        {
            .application_bar_height = 40.0f,
            .context_toolbar_height = 40.0f,
            .activity_rail_width = 48.0f,
            .explorer_width = 256.0f,
            .explorer_minimum_width = 240.0f,
            .explorer_maximum_width = 280.0f,
            .splitter_width = 8.0f,
            .inspector_width = 320.0f,
            .inspector_minimum_width = 300.0f,
            .inspector_maximum_width = 360.0f,
            .workspace_minimum_width = 560.0f,
            .operation_tray_minimum_height = 160.0f,
            .operation_tray_maximum_height = 240.0f,
            .operation_strip_height = 32.0f,
            .status_bar_height = 24.0f,
        },
    .explorer =
        {
            .search_height = 32.0f,
            .summary_minimum_height = 40.0f,
            .tree_indent = 16.0f,
        },
    .inspector =
        {
            .label_width = 112.0f,
            .information_row_minimum_height = 32.0f,
            .information_metric_row_minimum_height = 72.0f,
            .compass_minimum_height = 104.0f,
        },
    .settings =
        {
            .width = 880.0f,
            .height = 560.0f,
            .minimum_width = 720.0f,
            .minimum_height = 480.0f,
            .inset = 16.0f,
            .title_bar_height = 48.0f,
        },
};

float Resolve(const float logical_pixels, const float scale) {
  return std::round(logical_pixels * scale);
}

} // namespace

const LayoutMetrics &LogicalLayoutMetrics() { return kLogicalMetrics; }

LayoutMetrics ResolveLayoutMetrics(const UiEnvironment &environment) {
  const float scale =
      std::isfinite(environment.layout_scale) && environment.layout_scale > 0.0f
          ? environment.layout_scale
          : 1.0f;
  const float body_em =
      std::isfinite(environment.base_font_em) && environment.base_font_em > 0.0f
          ? environment.base_font_em
          : kDefaultBodyFontEm;
  LayoutMetrics metrics = kLogicalMetrics;
  const auto resolve = [scale](float &value) { value = Resolve(value, scale); };

  resolve(metrics.spacing.condensed);
  resolve(metrics.spacing.space01);
  resolve(metrics.spacing.space02);
  resolve(metrics.spacing.space03);
  resolve(metrics.spacing.space04);
  resolve(metrics.spacing.space05);
  resolve(metrics.spacing.space06);
  resolve(metrics.spacing.space07);
  resolve(metrics.spacing.space08);
  resolve(metrics.spacing.space09);
  resolve(metrics.spacing.space10);
  resolve(metrics.spacing.space11);
  resolve(metrics.spacing.space12);
  resolve(metrics.spacing.space13);

  resolve(metrics.geometry.border);
  resolve(metrics.geometry.focus_ring);
  resolve(metrics.geometry.control_radius);
  resolve(metrics.geometry.surface_radius);
  resolve(metrics.geometry.child_window_radius);
  resolve(metrics.geometry.icon);
  resolve(metrics.geometry.activity_icon);
  resolve(metrics.geometry.progress_height);
  resolve(metrics.geometry.compact_target);
  resolve(metrics.geometry.control_height);
  resolve(metrics.geometry.row_height);
  resolve(metrics.geometry.panel_header_height);

  metrics.typography.body_font_height =
      body_em * kNotoVerticalMetricsPerEm * scale;
  metrics.typography.section_heading_font_height =
      metrics.typography.body_font_height * 1.125f;
  metrics.typography.settings_title_font_height =
      metrics.typography.body_font_height * 1.25f;
  metrics.typography.page_title_font_height =
      metrics.typography.body_font_height * 1.5f;

  resolve(metrics.menu.popup_padding_horizontal);
  resolve(metrics.menu.popup_padding_vertical);
  resolve(metrics.menu.popup_width);
  resolve(metrics.menu.trigger_rounding);

  resolve(metrics.shell.application_bar_height);
  resolve(metrics.shell.context_toolbar_height);
  resolve(metrics.shell.activity_rail_width);
  resolve(metrics.shell.explorer_width);
  resolve(metrics.shell.explorer_minimum_width);
  resolve(metrics.shell.explorer_maximum_width);
  resolve(metrics.shell.splitter_width);
  resolve(metrics.shell.inspector_width);
  resolve(metrics.shell.inspector_minimum_width);
  resolve(metrics.shell.inspector_maximum_width);
  resolve(metrics.shell.workspace_minimum_width);
  resolve(metrics.shell.operation_tray_minimum_height);
  resolve(metrics.shell.operation_tray_maximum_height);
  resolve(metrics.shell.operation_strip_height);
  resolve(metrics.shell.status_bar_height);

  resolve(metrics.explorer.search_height);
  resolve(metrics.explorer.summary_minimum_height);
  resolve(metrics.explorer.tree_indent);

  resolve(metrics.inspector.label_width);
  resolve(metrics.inspector.information_row_minimum_height);
  resolve(metrics.inspector.information_metric_row_minimum_height);
  resolve(metrics.inspector.compass_minimum_height);

  resolve(metrics.settings.width);
  resolve(metrics.settings.height);
  resolve(metrics.settings.minimum_width);
  resolve(metrics.settings.minimum_height);
  resolve(metrics.settings.inset);
  resolve(metrics.settings.title_bar_height);

  const float padded_body =
      metrics.typography.body_font_height + metrics.spacing.space02 * 2.0f;
  const float compact_body =
      metrics.typography.body_font_height + metrics.spacing.space01 * 2.0f;
  metrics.geometry.control_height =
      std::max(metrics.geometry.control_height, std::ceil(padded_body));
  metrics.geometry.row_height =
      std::max(metrics.geometry.row_height, std::ceil(padded_body));
  metrics.geometry.compact_target =
      std::max(metrics.geometry.compact_target, std::ceil(compact_body));
  metrics.shell.application_bar_height =
      std::max(metrics.shell.application_bar_height, std::ceil(padded_body));
  metrics.shell.context_toolbar_height =
      std::max(metrics.shell.context_toolbar_height, std::ceil(padded_body));
  metrics.shell.operation_strip_height =
      std::max(metrics.shell.operation_strip_height, std::ceil(compact_body));
  metrics.shell.status_bar_height =
      std::max(metrics.shell.status_bar_height, std::ceil(compact_body));
  metrics.explorer.search_height =
      std::max(metrics.explorer.search_height, std::ceil(padded_body));
  return metrics;
}

LayoutMetrics CurrentLayoutMetrics() {
  return ResolveLayoutMetrics(CurrentUiEnvironment());
}

} // namespace fancy_ui
