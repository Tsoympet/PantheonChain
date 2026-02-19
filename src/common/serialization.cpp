#include "serialization.h"

#include <array>
#include <sstream>
#include <vector>

namespace pantheon::common {

std::string EncodeCommitment(const Commitment& commitment) {
    std::ostringstream stream;
    stream << SourceChainName(commitment.source_chain) << ':' << commitment.epoch << ':'
           << commitment.finalized_height << ':' << commitment.finalized_block_hash << ':'
           << commitment.state_root << ':' << commitment.validator_set_hash << ':'
           << commitment.upstream_commitment_hash << ':';

    for (std::size_t i = 0; i < commitment.signatures.size(); ++i) {
        const auto& signature = commitment.signatures[i];
        if (i != 0) {
            stream << ',';
        }
        stream << signature.validator_id << '|' << signature.stake_weight << '|'
               << signature.signature;
    }
    return stream.str();
}

CommitmentValidationResult DecodeCommitment(const std::string& encoded,
                                            Commitment& commitment) {
    std::array<std::string, 8> fields;
    std::size_t offset = 0;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const std::size_t delimiter = encoded.find(':', offset);
        if (delimiter == std::string::npos) {
            if (i != fields.size() - 1) {
                return {false, "encoded commitment must contain 8 colon-delimited fields"};
            }
            fields[i] = encoded.substr(offset);
            offset = encoded.size();
            continue;
        }

        fields[i] = encoded.substr(offset, delimiter - offset);
        offset = delimiter + 1;
    }

    if (offset != encoded.size()) {
        return {false, "encoded commitment has trailing fields"};
    }

    if (fields[0] == "DRACHMA") {
        commitment.source_chain = SourceChain::DRACHMA;
    } else if (fields[0] == "OBOLOS") {
        commitment.source_chain = SourceChain::OBOLOS;
    } else {
        return {false, "unsupported source_chain in encoded commitment"};
    }

    try {
        commitment.epoch = std::stoull(fields[1]);
        commitment.finalized_height = std::stoull(fields[2]);
    } catch (const std::exception&) {
        return {false, "epoch and finalized_height must be unsigned integers"};
    }

    commitment.finalized_block_hash = fields[3];
    commitment.state_root = fields[4];
    commitment.validator_set_hash = fields[5];

    commitment.upstream_commitment_hash = fields[6];

    commitment.signatures.clear();
    std::size_t cursor = 0;
    while (cursor <= fields[7].size()) {
        const std::size_t next = fields[7].find(',', cursor);
        const std::string item =
            fields[7].substr(cursor, next == std::string::npos ? std::string::npos : next - cursor);
        if (!item.empty()) {
            const std::size_t first = item.find('|');
            const std::size_t second = first == std::string::npos ? std::string::npos : item.find('|', first + 1);
            if (first == std::string::npos || second == std::string::npos) {
                return {false, "invalid validator signature tuple encoding"};
            }

            FinalitySignature signature;
            signature.validator_id = item.substr(0, first);
            try {
                signature.stake_weight = std::stoull(item.substr(first + 1, second - first - 1));
            } catch (const std::exception&) {
                return {false, "validator stake_weight must be an unsigned integer"};
            }
            signature.signature = item.substr(second + 1);
            commitment.signatures.push_back(signature);
        }

        if (next == std::string::npos) {
            break;
        }
        cursor = next + 1;
    }

    return ValidatePayloadEncoding(commitment);
}

}  // namespace pantheon::common
