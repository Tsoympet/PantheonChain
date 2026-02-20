// ParthenonChain - Transaction Implementation
// Consensus-critical: UTXO transaction serialization and validation

#include "transaction.h"

#include <cstddef>
#include <cstring>
#include <map>
#include <set>

namespace parthenon {
namespace primitives {

// Compact size encoding (Bitcoin-style)
void WriteCompactSize(std::vector<uint8_t>& output, uint64_t size) {
    if (size < 253) {
        output.push_back(static_cast<uint8_t>(size));
    } else if (size <= 0xFFFF) {
        output.push_back(253);
        output.push_back(static_cast<uint8_t>(size));
        output.push_back(static_cast<uint8_t>(size >> 8));
    } else if (size <= 0xFFFFFFFF) {
        output.push_back(254);
        output.push_back(static_cast<uint8_t>(size));
        output.push_back(static_cast<uint8_t>(size >> 8));
        output.push_back(static_cast<uint8_t>(size >> 16));
        output.push_back(static_cast<uint8_t>(size >> 24));
    } else {
        output.push_back(255);
        for (int i = 0; i < 8; i++) {
            output.push_back(static_cast<uint8_t>(size >> (i * 8)));
        }
    }
}

uint64_t ReadCompactSize(const uint8_t*& input) {
    uint8_t first = *input++;
    if (first < 253) {
        return first;
    } else if (first == 253) {
        uint64_t size = input[0] | (static_cast<uint64_t>(input[1]) << 8);
        input += 2;
        return size;
    } else if (first == 254) {
        uint64_t size = input[0] | (static_cast<uint64_t>(input[1]) << 8) |
                        (static_cast<uint64_t>(input[2]) << 16) |
                        (static_cast<uint64_t>(input[3]) << 24);
        input += 4;
        return size;
    } else {
        uint64_t size = 0;
        for (int i = 0; i < 8; i++) {
            size |= static_cast<uint64_t>(input[i]) << (i * 8);
        }
        input += 8;
        return size;
    }
}

static bool ReadCompactSizeChecked(const uint8_t*& input, const uint8_t* end, uint64_t& size) {
    if (input >= end) {
        return false;
    }

    uint8_t first = *input++;
    if (first < 253) {
        size = first;
        return true;
    } else if (first == 253) {
        if (end - input < 2) {
            return false;
        }
        size = input[0] | (static_cast<uint64_t>(input[1]) << 8);
        input += 2;
        return true;
    } else if (first == 254) {
        if (end - input < 4) {
            return false;
        }
        size = input[0] | (static_cast<uint64_t>(input[1]) << 8) |
               (static_cast<uint64_t>(input[2]) << 16) | (static_cast<uint64_t>(input[3]) << 24);
        input += 4;
        return true;
    } else {
        if (end - input < 8) {
            return false;
        }
        size = 0;
        for (int i = 0; i < 8; i++) {
            size |= static_cast<uint64_t>(input[i]) << (i * 8);
        }
        input += 8;
        return true;
    }
}

// OutPoint serialization
void OutPoint::Serialize(std::vector<uint8_t>& output) const {
    // 32 bytes txid
    output.insert(output.end(), txid.begin(), txid.end());

    // 4 bytes vout (little-endian)
    output.push_back(static_cast<uint8_t>(vout));
    output.push_back(static_cast<uint8_t>(vout >> 8));
    output.push_back(static_cast<uint8_t>(vout >> 16));
    output.push_back(static_cast<uint8_t>(vout >> 24));
}

OutPoint OutPoint::Deserialize(const uint8_t* input) {
    OutPoint result;

    // 32 bytes txid
    std::copy(input, input + 32, result.txid.begin());
    input += 32;

    // 4 bytes vout
    result.vout = input[0] | (static_cast<uint32_t>(input[1]) << 8) |
                  (static_cast<uint32_t>(input[2]) << 16) | (static_cast<uint32_t>(input[3]) << 24);

    return result;
}

// TxInput serialization
void TxInput::Serialize(std::vector<uint8_t>& output) const {
    prevout.Serialize(output);

    WriteCompactSize(output, signature_script.size());
    output.insert(output.end(), signature_script.begin(), signature_script.end());

    output.push_back(static_cast<uint8_t>(sequence));
    output.push_back(static_cast<uint8_t>(sequence >> 8));
    output.push_back(static_cast<uint8_t>(sequence >> 16));
    output.push_back(static_cast<uint8_t>(sequence >> 24));
}

std::optional<TxInput> TxInput::Deserialize(const uint8_t*& input, const uint8_t* end) {
    TxInput result;

    if (end - input < 36) {
        return std::nullopt;
    }
    result.prevout = OutPoint::Deserialize(input);
    input += 36;  // 32 + 4

    uint64_t script_len = 0;
    if (!ReadCompactSizeChecked(input, end, script_len)) {
        return std::nullopt;
    }
    if (end - input < static_cast<ptrdiff_t>(script_len + 4)) {
        return std::nullopt;
    }
    result.signature_script.resize(script_len);
    std::copy(input, input + script_len, result.signature_script.begin());
    input += script_len;

    result.sequence = input[0] | (static_cast<uint32_t>(input[1]) << 8) |
                      (static_cast<uint32_t>(input[2]) << 16) |
                      (static_cast<uint32_t>(input[3]) << 24);
    input += 4;

    return result;
}

// TxOutput serialization
void TxOutput::Serialize(std::vector<uint8_t>& output) const {
    // Asset ID (1 byte) + Amount (8 bytes)
    uint8_t asset_amount[9];
    value.Serialize(asset_amount);
    output.insert(output.end(), asset_amount, asset_amount + 9);

    // Pubkey script
    WriteCompactSize(output, pubkey_script.size());
    output.insert(output.end(), pubkey_script.begin(), pubkey_script.end());
}

std::optional<TxOutput> TxOutput::Deserialize(const uint8_t*& input, const uint8_t* end) {
    TxOutput result;

    if (end - input < 9) {
        return std::nullopt;
    }
    result.value = AssetAmount::Deserialize(input);
    input += 9;

    uint64_t script_len = 0;
    if (!ReadCompactSizeChecked(input, end, script_len)) {
        return std::nullopt;
    }
    if (end - input < static_cast<ptrdiff_t>(script_len)) {
        return std::nullopt;
    }
    result.pubkey_script.resize(script_len);
    std::copy(input, input + script_len, result.pubkey_script.begin());
    input += script_len;

    return result;
}

// Transaction methods
std::vector<uint8_t> Transaction::Serialize() const {
    std::vector<uint8_t> result;

    // Version (4 bytes)
    result.push_back(static_cast<uint8_t>(version));
    result.push_back(static_cast<uint8_t>(version >> 8));
    result.push_back(static_cast<uint8_t>(version >> 16));
    result.push_back(static_cast<uint8_t>(version >> 24));

    // Input count
    WriteCompactSize(result, inputs.size());

    // Inputs
    for (const auto& input : inputs) {
        input.Serialize(result);
    }

    // Output count
    WriteCompactSize(result, outputs.size());

    // Outputs
    for (const auto& output : outputs) {
        output.Serialize(result);
    }

    // Locktime (4 bytes)
    result.push_back(static_cast<uint8_t>(locktime));
    result.push_back(static_cast<uint8_t>(locktime >> 8));
    result.push_back(static_cast<uint8_t>(locktime >> 16));
    result.push_back(static_cast<uint8_t>(locktime >> 24));

    return result;
}

std::optional<Transaction> Transaction::Deserialize(const uint8_t* data, size_t len) {
    if (len < 10)
        return std::nullopt;  // Minimum transaction size

    Transaction tx;
    const uint8_t* ptr = data;
    const uint8_t* end = data + len;

    // Version
    if (end - ptr < 4) {
        return std::nullopt;
    }
    tx.version = ptr[0] | (static_cast<uint32_t>(ptr[1]) << 8) |
                 (static_cast<uint32_t>(ptr[2]) << 16) | (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;

    // Input count
    uint64_t input_count = 0;
    if (!ReadCompactSizeChecked(ptr, end, input_count)) {
        return std::nullopt;
    }
    if (input_count > 100000)
        return std::nullopt;  // Sanity check

    // Inputs
    for (uint64_t i = 0; i < input_count; i++) {
        auto input_opt = TxInput::Deserialize(ptr, end);
        if (!input_opt) {
            return std::nullopt;
        }
        tx.inputs.push_back(*input_opt);
    }

    // Output count
    uint64_t output_count = 0;
    if (!ReadCompactSizeChecked(ptr, end, output_count)) {
        return std::nullopt;
    }
    if (output_count > 100000)
        return std::nullopt;  // Sanity check

    // Outputs
    for (uint64_t i = 0; i < output_count; i++) {
        auto output_opt = TxOutput::Deserialize(ptr, end);
        if (!output_opt) {
            return std::nullopt;
        }
        tx.outputs.push_back(*output_opt);
    }

    // Locktime
    if (end - ptr < 4) {
        return std::nullopt;
    }
    tx.locktime = ptr[0] | (static_cast<uint32_t>(ptr[1]) << 8) |
                  (static_cast<uint32_t>(ptr[2]) << 16) | (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;
    return tx;
}

std::array<uint8_t, 32> Transaction::GetTxID() const {
    auto serialized = Serialize();
    return crypto::SHA256d::Hash256d(serialized);
}

std::vector<uint8_t> Transaction::SerializeForSigning(size_t input_index) const {
    // Create a copy and clear all input scripts except the one being signed
    std::vector<uint8_t> result;

    // Version
    result.push_back(static_cast<uint8_t>(version));
    result.push_back(static_cast<uint8_t>(version >> 8));
    result.push_back(static_cast<uint8_t>(version >> 16));
    result.push_back(static_cast<uint8_t>(version >> 24));

    // Input count
    WriteCompactSize(result, inputs.size());

    // Inputs (with signature scripts removed)
    for (size_t i = 0; i < inputs.size(); i++) {
        TxInput input_copy = inputs[i];
        if (i != input_index) {
            input_copy.signature_script.clear();
        }
        input_copy.Serialize(result);
    }

    // Outputs
    WriteCompactSize(result, outputs.size());
    for (const auto& output : outputs) {
        output.Serialize(result);
    }

    // Locktime
    result.push_back(static_cast<uint8_t>(locktime));
    result.push_back(static_cast<uint8_t>(locktime >> 8));
    result.push_back(static_cast<uint8_t>(locktime >> 16));
    result.push_back(static_cast<uint8_t>(locktime >> 24));

    return result;
}

std::array<uint8_t, 32> Transaction::GetSignatureHash(size_t input_index) const {
    auto serialized = SerializeForSigning(input_index);
    return crypto::SHA256::Hash256(serialized.data(), serialized.size());
}

bool Transaction::IsValid() const {
    // Must have inputs and outputs
    if (inputs.empty() || outputs.empty()) {
        return false;
    }

    // All outputs must be valid
    for (const auto& output : outputs) {
        if (!output.IsValid()) {
            return false;
        }
    }

    // Check for duplicate inputs (no double-spending in same transaction)
    std::set<OutPoint> seen_inputs;
    for (const auto& input : inputs) {
        if (!IsCoinbase()) {
            if (seen_inputs.count(input.prevout) > 0) {
                return false;  // Duplicate input
            }
            seen_inputs.insert(input.prevout);
        }
    }

    // Asset conservation check (inputs must be >= outputs for each asset)
    // Note: This requires UTXO set to verify, so we only do structural validation here
    // Full validation happens in the validation module

    return true;
}

}  // namespace primitives
}  // namespace parthenon
