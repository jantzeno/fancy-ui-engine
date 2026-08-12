#include "native_panel_contracts.hpp"

#include <utility>

namespace fancy_ui::gallery {

std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContractsChunk0();
std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContractsChunk1();
std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContractsChunk2();
std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContractsChunk3();
std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContractsChunk4();
std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContractsChunk5();
std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContractsChunk6();
std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContractsChunk7();

std::vector<steppenface::PanelContractView>
BuildCanonicalPanelAuditContracts() {
  std::vector<steppenface::PanelContractView> contracts;
  const auto append = [&contracts](auto chunk) {
    for (auto &contract : chunk) {
      contracts.push_back(std::move(contract));
    }
  };
  append(BuildCanonicalPanelAuditContractsChunk0());
  append(BuildCanonicalPanelAuditContractsChunk1());
  append(BuildCanonicalPanelAuditContractsChunk2());
  append(BuildCanonicalPanelAuditContractsChunk3());
  append(BuildCanonicalPanelAuditContractsChunk4());
  append(BuildCanonicalPanelAuditContractsChunk5());
  append(BuildCanonicalPanelAuditContractsChunk6());
  append(BuildCanonicalPanelAuditContractsChunk7());
  return contracts;
}

} // namespace fancy_ui::gallery
