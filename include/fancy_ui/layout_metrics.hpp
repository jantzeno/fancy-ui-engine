#pragma once

#include "fancy_ui/ui_environment.hpp"

namespace fancy_ui {

/**
 * Canonical layout values expressed in logical pixels.
 *
 * Persisted panel dimensions use these logical units. Call
 * ResolveLayoutMetrics() at the rendering boundary to obtain rounded physical
 * pixels for the active UI scale.
 */
struct LayoutMetrics {
  struct Spacing {
    float condensed;
    float space01;
    float space02;
    float space03;
    float space04;
    float space05;
    float space06;
    float space07;
    float space08;
    float space09;
    float space10;
    float space11;
    float space12;
    float space13;
  } spacing;

  struct Geometry {
    float border;
    float focus_ring;
    float control_radius;
    float surface_radius;
    float child_window_radius;
    float icon;
    float activity_icon;
    float progress_height;
    float compact_target;
    float control_height;
    float row_height;
    float panel_header_height;
  } geometry;

  struct Typography {
    float body_font_height;
    float section_heading_font_height;
    float settings_title_font_height;
    float page_title_font_height;
  } typography;

  struct Menu {
    float popup_padding_horizontal;
    float popup_padding_vertical;
    float popup_width;
    float trigger_rounding;
  } menu;

  struct Shell {
    float application_bar_height;
    float context_toolbar_height;
    float activity_rail_width;
    float explorer_width;
    float explorer_minimum_width;
    float explorer_maximum_width;
    float splitter_width;
    float inspector_width;
    float inspector_minimum_width;
    float inspector_maximum_width;
    float workspace_minimum_width;
    float operation_tray_minimum_height;
    float operation_tray_maximum_height;
    float operation_strip_height;
    float status_bar_height;
  } shell;

  struct Explorer {
    float search_height;
    float summary_minimum_height;
    float tree_indent;
  } explorer;

  struct Inspector {
    float label_width;
    float stack_breakpoint;
    float section_header_height;
    float information_row_minimum_height;
    float information_metric_row_minimum_height;
    float compass_minimum_height;
  } inspector;

  struct Settings {
    float width;
    float height;
    float minimum_width;
    float minimum_height;
    float inset;
    float title_bar_height;
  } settings;
};

[[nodiscard]] const LayoutMetrics &LogicalLayoutMetrics();
[[nodiscard]] LayoutMetrics
ResolveLayoutMetrics(const UiEnvironment &environment);
[[nodiscard]] LayoutMetrics CurrentLayoutMetrics();

} // namespace fancy_ui
