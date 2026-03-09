#pragma once
// bridge_test_helpers.h — Shared helpers for adversarial bridge tests.
//
// WARNING: This file is for TEST USE ONLY.  The private key defined here
// (kBridgeTestPrivKey) is publicly known; it must NEVER be used in production.
//
// Provides:
//  - A deterministic test private key (kBridgeTestPrivKey)
//  - bridge_sign_message()  — appends a valid ValidatorSignature to a message
//  - setup_bridge_state()   — configures a BridgeState with the test trusted key
//
// These helpers ensure tests exercise the real VerifyValidatorQuorum path while
// remaining reproducible.

#ifdef PANTHEON_PRODUCTION_BUILD
#  error "bridge_test_helpers.h must not be included in production builds."
#endif

#include "bridge/cross_chain_message.h"
#include "bridge/l1_l2/l1_l2_bridge.h"
#include "bridge/l2_l3/l2_l3_bridge.h"
#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include <array>
#include <cstdint>
#include <vector>

namespace bridge_test {

// Deterministic test private key (non-zero, well below curve order).
// seckey = 0x00...0001 (big-endian, value = 1)
static const parthenon::crypto::Schnorr::PrivateKey kBridgeTestPrivKey = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
};

// Derive the x-only (32-byte BIP-340) public key from kBridgeTestPrivKey.
inline parthenon::crypto::Schnorr::PublicKey bridge_test_pubkey()
{
    auto opt = parthenon::crypto::Schnorr::GetPublicKey(kBridgeTestPrivKey);
    // key is always valid; abort loudly if stub misbehaves
    if (!opt) { __builtin_trap(); }
    return *opt;
}

// Compute the canonical message commitment hash (same formula as VerifyValidatorQuorum).
// SHA256d(origin_chain_id_LE32 || dest_chain_id_LE32 || nonce_LE64
//         || payload_hash_32   || state_root_32)
inline pantheon::bridge::Hash256 bridge_commit_hash(
    const pantheon::bridge::CrossChainMessage& msg)
{
    std::vector<uint8_t> data;
    data.reserve(80);

    auto push_le32 = [&](uint32_t v) {
        for (int i = 0; i < 4; ++i)
            data.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    };
    auto push_le64 = [&](uint64_t v) {
        for (int i = 0; i < 8; ++i)
            data.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    };

    push_le32(static_cast<uint32_t>(msg.origin_chain_id));
    push_le32(static_cast<uint32_t>(msg.destination_chain_id));
    push_le64(msg.message_nonce);
    data.insert(data.end(), msg.payload_hash.begin(), msg.payload_hash.end());
    data.insert(data.end(), msg.state_root.begin(), msg.state_root.end());

    return parthenon::crypto::SHA256d::Hash256d(data.data(), data.size());
}

// Sign a CrossChainMessage with the test private key and append the
// resulting ValidatorSignature to message.validator_signatures.
inline void bridge_sign_message(pantheon::bridge::CrossChainMessage& msg)
{
    pantheon::bridge::Hash256 commit = bridge_commit_hash(msg);
    auto sig_opt = parthenon::crypto::Schnorr::Sign(kBridgeTestPrivKey, commit.data());
    if (!sig_opt) { __builtin_trap(); }
    auto pk_opt = parthenon::crypto::Schnorr::GetPublicKey(kBridgeTestPrivKey);
    if (!pk_opt) { __builtin_trap(); }

    pantheon::bridge::ValidatorSignature vs;
    vs.signature = *sig_opt;
    // ValidatorSignature::public_key is 33-byte compressed; prefix 0x02 = even y.
    vs.public_key[0] = 0x02;
    std::copy(pk_opt->begin(), pk_opt->end(), vs.public_key.begin() + 1);
    msg.validator_signatures.push_back(vs);
}

// Configure a L1↔L2 BridgeState to trust the test validator and require 1 signature.
inline void setup_bridge_state(pantheon::bridge::l1_l2::BridgeState& state)
{
    state.trusted_validator_pubkeys.insert(bridge_test_pubkey());
    state.min_validator_sigs = 1;
}

// Configure a L2↔L3 BridgeState to trust the test validator and require 1 signature.
inline void setup_bridge_state(pantheon::bridge::l2_l3::BridgeState& state)
{
    state.trusted_validator_pubkeys.insert(bridge_test_pubkey());
    state.min_validator_sigs = 1;
}

}  // namespace bridge_test
