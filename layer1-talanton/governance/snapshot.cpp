#include "snapshot.h"

namespace parthenon {
namespace governance {

bool SnapshotRegistry::CreateSnapshot(
        uint64_t proposal_id,
        uint64_t block_height,
        const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>& powers) {
    if (snapshots_.count(proposal_id)) return false;  // already exists

    Snapshot snap;
    snap.proposal_id  = proposal_id;
    snap.block_height = block_height;
    snap.total_power  = 0;

    for (const auto& [addr, power] : powers) {
        if (power == 0) continue;  // skip zero-power entries
        SnapshotEntry entry;
        entry.address      = addr;
        entry.voting_power = power;
        snap.entries.push_back(entry);
        snap.total_power += power;
    }

    snapshots_[proposal_id] = std::move(snap);
    return true;
}

uint64_t SnapshotRegistry::GetSnapshotPower(
        uint64_t proposal_id,
        const std::vector<uint8_t>& address) const {
    auto it = snapshots_.find(proposal_id);
    if (it == snapshots_.end()) return 0;

    for (const auto& entry : it->second.entries) {
        if (entry.address == address) return entry.voting_power;
    }
    return 0;
}

bool SnapshotRegistry::HasSnapshot(uint64_t proposal_id) const {
    return snapshots_.count(proposal_id) > 0;
}

uint64_t SnapshotRegistry::GetSnapshotBlock(uint64_t proposal_id) const {
    auto it = snapshots_.find(proposal_id);
    return (it != snapshots_.end()) ? it->second.block_height : 0;
}

uint64_t SnapshotRegistry::GetSnapshotTotalPower(uint64_t proposal_id) const {
    auto it = snapshots_.find(proposal_id);
    return (it != snapshots_.end()) ? it->second.total_power : 0;
}

std::optional<SnapshotRegistry::Snapshot>
SnapshotRegistry::GetSnapshot(uint64_t proposal_id) const {
    auto it = snapshots_.find(proposal_id);
    if (it == snapshots_.end()) return std::nullopt;
    return it->second;
}

}  // namespace governance
}  // namespace parthenon
