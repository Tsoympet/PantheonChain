#include "common/commitments.h"
#include "common/serialization.h"
#include "talanton/l1_commitment_validator.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

namespace {

void PrintUsage() {
    std::cerr
        << "Usage: pantheon-relayer-l2 --commitment=<encoded> --active-stake=<value> "
           "--last-finalized-height=<value>\n"
        << "Encoded format: DRACHMA:epoch:finalized_height:finalized_block_hash:state_root:"
           "validator_set_hash:validator_id|stake|signature(,...)\n";
}

std::optional<uint64_t> ParseUint(const std::string& value) {
    try {
        return std::stoull(value);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    std::optional<std::string> encoded_commitment;
    std::optional<uint64_t> active_stake;
    std::optional<uint64_t> last_finalized_height;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.rfind("--commitment=", 0) == 0) {
            encoded_commitment = arg.substr(13);
        } else if (arg.rfind("--active-stake=", 0) == 0) {
            active_stake = ParseUint(arg.substr(15));
        } else if (arg.rfind("--last-finalized-height=", 0) == 0) {
            last_finalized_height = ParseUint(arg.substr(24));
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            PrintUsage();
            return 1;
        }
    }

    if (!encoded_commitment || !active_stake || !last_finalized_height) {
        PrintUsage();
        return 1;
    }

    pantheon::common::Commitment commitment{};
    auto decode_result = pantheon::common::DecodeCommitment(*encoded_commitment, commitment);
    if (!decode_result.valid) {
        std::cerr << "pantheon-relayer-l2 decode error: " << decode_result.reason << std::endl;
        return 1;
    }

    auto validate_result = pantheon::talanton::ValidateL2Commit(
        commitment, pantheon::talanton::L2AnchorState{*last_finalized_height}, *active_stake);
    if (!validate_result.valid) {
        std::cerr << "pantheon-relayer-l2 rejected commitment: " << validate_result.reason
                  << std::endl;
        return 1;
    }

    std::cout << "pantheon-relayer-l2 relayed: " << pantheon::common::EncodeCommitment(commitment)
              << std::endl;
    return 0;
}
