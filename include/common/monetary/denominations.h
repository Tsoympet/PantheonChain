#pragma once

#include "primitives/asset.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace parthenon::common::monetary {

struct DenominationDefinition {
    std::string name;
    std::string symbol;
    parthenon::primitives::AssetID asset;
    uint64_t token_units_numerator;
    uint64_t token_units_denominator;
    uint32_t allowed_decimals;
    bool allow_input;
    bool approximate_display;
    std::vector<std::string> aliases;
};

const std::vector<DenominationDefinition>& GetAtticDisplayDenominations(
    parthenon::primitives::AssetID asset);

const DenominationDefinition* ResolveDenomination(parthenon::primitives::AssetID asset,
                                                  const std::string& name_or_alias);

std::string DefaultDenominationName(parthenon::primitives::AssetID asset);

}  // namespace parthenon::common::monetary

