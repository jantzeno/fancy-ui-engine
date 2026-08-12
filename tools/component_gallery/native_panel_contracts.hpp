#pragma once

#include "fancy_ui/steppenface/application_view.hpp"

#include <vector>

namespace fancy_ui::gallery {

[[nodiscard]] std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContracts();

} // namespace fancy_ui::gallery
