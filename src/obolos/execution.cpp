#include "execution.h"

#include <algorithm>

namespace pantheon::obolos {

ExecutionResult ExecuteEvmLikeCall(const std::string& payload, uint64_t gas_limit,
                                   uint64_t base_fee_per_gas) {
    const uint64_t intrinsic_gas = 21000;
    const uint64_t payload_gas = static_cast<uint64_t>(payload.size()) * 16;
    const uint64_t total_gas = intrinsic_gas + payload_gas;

    if (base_fee_per_gas == 0 || gas_limit < total_gas) {
        return {false, std::min(gas_limit, total_gas), "out of gas"};
    }

    return {true, total_gas, "0x"};
}

}  // namespace pantheon::obolos
