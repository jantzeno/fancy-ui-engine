#pragma once

#include "fancy_ui/components/button.hpp"

#include <optional>
#include <string_view>

namespace fancy_ui {

struct SectionSpec {
  std::string_view id;
  std::string_view heading;
  std::string_view summary;
  bool open = true;
  bool focused = false;
  bool separated = false;
  std::optional<ButtonSpec> header_action;
};

struct SectionResult {
  bool open = false;
  bool open_changed = false;
  bool header_action_activated = false;
};

[[nodiscard]] SectionResult BeginSection(const SectionSpec &spec);
void EndSection(const SectionResult &result);

} // namespace fancy_ui
