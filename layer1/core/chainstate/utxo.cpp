// ParthenonChain - UTXO Set Implementation
// Consensus-critical: Must be deterministic

#include "utxo.h"

namespace parthenon {
namespace chainstate {

void UTXOSet::AddCoin(const primitives::OutPoint& outpoint, const Coin& coin) {
    utxos_[outpoint] = coin;
}

bool UTXOSet::SpendCoin(const primitives::OutPoint& outpoint) {
    auto it = utxos_.find(outpoint);
    if (it == utxos_.end()) {
        return false;  // Coin not found
    }
    utxos_.erase(it);
    return true;
}

std::optional<Coin> UTXOSet::GetCoin(const primitives::OutPoint& outpoint) const {
    auto it = utxos_.find(outpoint);
    if (it == utxos_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool UTXOSet::HaveCoin(const primitives::OutPoint& outpoint) const {
    return utxos_.find(outpoint) != utxos_.end();
}

}  // namespace chainstate
}  // namespace parthenon
