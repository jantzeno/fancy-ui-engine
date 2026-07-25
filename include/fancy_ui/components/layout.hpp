#pragma once

#include <string_view>

namespace fancy_ui {

struct SectionSpec {
  std::string_view id;
  std::string_view heading;
  bool initially_open = true;
};

struct SectionResult {
  bool visible = false;
  bool open = false;
};

[[nodiscard]] SectionResult BeginSection(const SectionSpec &spec);
void EndSection(const SectionResult &result);

} // namespace fancy_ui
