#pragma once

#include "primitives/asset.h"

#include <cstdint>
#include <optional>
#include <string>

namespace parthenon::common::monetary {

enum class RoundingMode {
    Floor,
    Bankers,
};

struct AmountView {
    uint64_t amount_raw;
    std::string amount;
    std::string amount_formatted;
    std::string token;
    std::string denom_used;
    std::optional<std::string> dual_display;
    bool approximate;
};

constexpr uint32_t TAL_DECIMALS = 8;
constexpr uint32_t DR_DECIMALS = 8;
constexpr uint32_t OB_DECIMALS = 8;

constexpr uint64_t TAL_BASE_UNIT = 100000000ULL;   // talantonion
constexpr uint64_t DR_BASE_UNIT = 100000000ULL;    // drachmion
constexpr uint64_t OB_BASE_UNIT = 100000000ULL;    // obolion

constexpr uint64_t RATIO_DR_PER_TAL = 6000ULL;
constexpr uint64_t RATIO_OB_PER_DR = 6ULL;
constexpr uint64_t RATIO_OB_PER_TAL = 36000ULL;

static_assert(RATIO_DR_PER_TAL * RATIO_OB_PER_DR == RATIO_OB_PER_TAL,
              "Ancient Greek denomination ratios must remain internally consistent.");

constexpr uint32_t GetTokenDecimals(parthenon::primitives::AssetID asset) {
    switch (asset) {
        case parthenon::primitives::AssetID::TALANTON:
            return TAL_DECIMALS;
        case parthenon::primitives::AssetID::DRACHMA:
            return DR_DECIMALS;
        case parthenon::primitives::AssetID::OBOLOS:
            return OB_DECIMALS;
        default:
            return TAL_DECIMALS;
    }
}

constexpr uint64_t GetTokenBaseUnit(parthenon::primitives::AssetID asset) {
    switch (asset) {
        case parthenon::primitives::AssetID::TALANTON:
            return TAL_BASE_UNIT;
        case parthenon::primitives::AssetID::DRACHMA:
            return DR_BASE_UNIT;
        case parthenon::primitives::AssetID::OBOLOS:
            return OB_BASE_UNIT;
        default:
            return TAL_BASE_UNIT;
    }
}

std::optional<uint64_t> ConvertTalToDr(uint64_t tal_raw);
std::optional<uint64_t> ConvertDrToOb(uint64_t dr_raw);
std::optional<uint64_t> ConvertTalToOb(uint64_t tal_raw);
std::optional<uint64_t> ConvertDrToTal(uint64_t dr_raw, RoundingMode rounding = RoundingMode::Floor);
std::optional<uint64_t> ConvertObToDr(uint64_t ob_raw, RoundingMode rounding = RoundingMode::Floor);
std::optional<uint64_t> ConvertObToTal(uint64_t ob_raw, RoundingMode rounding = RoundingMode::Floor);

std::string FormatAmount(uint64_t amount_raw, parthenon::primitives::AssetID asset);
std::optional<std::string> FormatAmountWithDenomination(uint64_t amount_raw,
                                                        parthenon::primitives::AssetID asset,
                                                        const std::string& denomination,
                                                        bool* approximate = nullptr);
std::optional<uint64_t> ParseDisplayAmount(const std::string& value,
                                           parthenon::primitives::AssetID asset);
std::optional<uint64_t> ParseDisplayAmountWithDenomination(const std::string& value,
                                                           parthenon::primitives::AssetID asset,
                                                           const std::string& denomination,
                                                           std::string* error = nullptr);
AmountView BuildAmountView(uint64_t amount_raw, parthenon::primitives::AssetID asset,
                           const std::string& denomination = "", bool dual_display = false);

std::string MonetarySpecHash();
std::string MonetarySpecPayload();

bool ValidateMonetaryInvariants();

}  // namespace parthenon::common::monetary
