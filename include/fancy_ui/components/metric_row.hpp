#pragma once

#include <span>
#include <string_view>

namespace fancy_ui {

struct MetricValue {
  std::string_view label;
  std::string_view value;
  bool wide = false;
  bool stacked = false;
};

struct MetricRowSpec {
  std::string_view id;
  std::string_view label;
  std::span<const MetricValue> metrics;
  float minimum_height = 32.0f;
};

void MetricRow(const MetricRowSpec &spec);

} // namespace fancy_ui
