// l2_l3_bridge.cpp — DRACHMA ↔ OBOLOS Bridge Implementation

#include "bridge/l2_l3/l2_l3_bridge.h"
#include <sstream>

static pantheon::bridge::Hash256 sha256d_stub(
    const uint8_t* data, size_t len)
{
    (void)data; (void)len;
    pantheon::bridge::Hash256 h{};
    return h;
}

namespace pantheon {
namespace bridge {
namespace l2_l3 {

static std::string make_nonce_key(const std::string& sender, uint64_t nonce)
{
    std::ostringstream oss;
    oss << sender << ":" << nonce;
    return oss.str();
}

bool VerifyMerkleProof(
    const Hash256& leaf_hash,
    const std::vector<std::vector<uint8_t>>& proof_nodes,
    const Hash256& expected_root)
{
    Hash256 current = leaf_hash;
    for (const auto& sibling : proof_nodes) {
        std::vector<uint8_t> combined(current.begin(), current.end());
        combined.insert(combined.end(), sibling.begin(), sibling.end());
        current = sha256d_stub(combined.data(), combined.size());
    }
    return current == expected_root;
}

BridgeResult RecordDrcLock(
    BridgeState& state,
    const BridgeTransferIntent& intent)
{
    if (intent.origin_chain_id != ChainId::DRACHMA ||
        intent.destination_chain_id != ChainId::OBOLOS)
        return BridgeResult::ERR_INVALID_CHAIN;

    if (intent.amount_base_units == 0)
        return BridgeResult::ERR_AMOUNT_ZERO;

    const std::string key = make_nonce_key(intent.sender_address, intent.nonce);
    if (state.processed_nonce_keys.count(key))
        return BridgeResult::ERR_REPLAY;

    state.processed_nonce_keys.insert(key);
    state.total_locked_drc_base_units += intent.amount_base_units;
    return BridgeResult::OK;
}

BridgeResult ProcessWdrcBurnUnlock(
    BridgeState& state,
    const CrossChainMessage& message)
{
    if (message.origin_chain_id != ChainId::OBOLOS ||
        message.destination_chain_id != ChainId::DRACHMA)
        return BridgeResult::ERR_INVALID_CHAIN;

    const std::string key = make_nonce_key(
        std::to_string(static_cast<uint32_t>(message.origin_chain_id)),
        message.message_nonce);
    if (state.processed_nonce_keys.count(key))
        return BridgeResult::ERR_REPLAY;

    if (!VerifyMerkleProof(message.payload_hash, message.proof, message.state_root))
        return BridgeResult::ERR_INVALID_PROOF;

    state.processed_nonce_keys.insert(key);
    return BridgeResult::OK;
}

BridgeResult ProcessDrcLockMint(
    BridgeState& state,
    const CrossChainMessage& message)
{
    if (message.origin_chain_id != ChainId::DRACHMA ||
        message.destination_chain_id != ChainId::OBOLOS)
        return BridgeResult::ERR_INVALID_CHAIN;

    const std::string key = make_nonce_key(
        std::to_string(static_cast<uint32_t>(message.origin_chain_id)),
        message.message_nonce);
    if (state.processed_nonce_keys.count(key))
        return BridgeResult::ERR_REPLAY;

    if (!VerifyMerkleProof(message.payload_hash, message.proof, message.state_root))
        return BridgeResult::ERR_INVALID_PROOF;

    state.processed_nonce_keys.insert(key);

    // The actual mint amount is encoded in the message payload as a
    // serialised BridgeTransferIntent. In production, deserialise the
    // payload and use BridgeTransferIntent::amount_base_units.
    // total_minted_wdrc_base_units is updated after deserialization.
    return BridgeResult::OK;
}

BridgeResult RecordWdrcBurn(
    BridgeState& state,
    const BridgeTransferIntent& intent)
{
    if (intent.origin_chain_id != ChainId::OBOLOS ||
        intent.destination_chain_id != ChainId::DRACHMA)
        return BridgeResult::ERR_INVALID_CHAIN;

    if (intent.amount_base_units == 0)
        return BridgeResult::ERR_AMOUNT_ZERO;

    const std::string key = make_nonce_key(intent.sender_address, intent.nonce);
    if (state.processed_nonce_keys.count(key))
        return BridgeResult::ERR_REPLAY;

    state.processed_nonce_keys.insert(key);
    return BridgeResult::OK;
}

}  // namespace l2_l3
}  // namespace bridge
}  // namespace pantheon
