#include "common/monetary/denominations.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace parthenon::common::monetary {
namespace {

std::string LowerTrimmed(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char c) {
                    return !std::isspace(c);
                }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char c) {
                    return !std::isspace(c);
                }).base(),
                value.end());

    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

const std::vector<DenominationDefinition>& TalDenoms() {
    static const std::vector<DenominationDefinition> denoms = {
        {"talanton", "TAL", parthenon::primitives::AssetID::TALANTON, 1, 1, 8, true, false,
         {"tal", "talanton", "talanta"}},
        {"mina", "MNA", parthenon::primitives::AssetID::TALANTON, 1, 60, 8, false, true,
         {"mina", "mina"}},
    };
    return denoms;
}

const std::vector<DenominationDefinition>& DrDenoms() {
    static const std::vector<DenominationDefinition> denoms = {
        {"drachma", "DR", parthenon::primitives::AssetID::DRACHMA, 1, 1, 8, true, false,
         {"dr", "drachma", "drachmas"}},
        {"obol", "OB", parthenon::primitives::AssetID::DRACHMA, 1, 6, 8, true, true,
         {"ob", "obol", "obolos"}},
        {"tetradrachm", "4DR", parthenon::primitives::AssetID::DRACHMA, 4, 1, 8, true, false,
         {"tetradrachm", "tetradrachma", "tetra"}},
        {"mina", "MNA", parthenon::primitives::AssetID::DRACHMA, 100, 1, 8, true, false,
         {"mina", "mina"}},
    };
    return denoms;
}

const std::vector<DenominationDefinition>& ObDenoms() {
    static const std::vector<DenominationDefinition> denoms = {
        {"obol", "OB", parthenon::primitives::AssetID::OBOLOS, 1, 1, 8, true, false,
         {"ob", "obol", "obolos"}},
        {"hemiobol", "1/2OB", parthenon::primitives::AssetID::OBOLOS, 1, 2, 8, false, false,
         {"hemiobol", "hemi-obol"}},
    };
    return denoms;
}

}  // namespace

const std::vector<DenominationDefinition>& GetAtticDisplayDenominations(
    parthenon::primitives::AssetID asset) {
    switch (asset) {
        case parthenon::primitives::AssetID::TALANTON:
            return TalDenoms();
        case parthenon::primitives::AssetID::DRACHMA:
            return DrDenoms();
        case parthenon::primitives::AssetID::OBOLOS:
            return ObDenoms();
        default:
            return TalDenoms();
    }
}

const DenominationDefinition* ResolveDenomination(parthenon::primitives::AssetID asset,
                                                  const std::string& name_or_alias) {
    const auto needle = LowerTrimmed(name_or_alias);
    if (needle.empty()) {
        return nullptr;
    }

    const auto& denoms = GetAtticDisplayDenominations(asset);
    for (const auto& denom : denoms) {
        if (LowerTrimmed(denom.name) == needle || LowerTrimmed(denom.symbol) == needle) {
            return &denom;
        }

        const auto it = std::find_if(denom.aliases.begin(), denom.aliases.end(),
                                     [&needle](const std::string& alias) {
                                         return LowerTrimmed(alias) == needle;
                                     });
        if (it != denom.aliases.end()) {
            return &denom;
        }
    }

    return nullptr;
}

std::string DefaultDenominationName(parthenon::primitives::AssetID asset) {
    const auto& denoms = GetAtticDisplayDenominations(asset);
    return denoms.empty() ? "" : denoms.front().name;
}

}  // namespace parthenon::common::monetary

