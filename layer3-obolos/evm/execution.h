#pragma once

#include <cstdint>
#include <string>

namespace pantheon::obolos {

struct ExecutionResult {
    bool success = false;
    uint64_t gas_used = 0;
    std::string output;
};

ExecutionResult ExecuteEvmLikeCall(const std::string& payload, uint64_t gas_limit,
                                   uint64_t base_fee_per_gas);

}  // namespace pantheon::obolos
