#include "common/monetary/units.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>

namespace parthenon::common::monetary {
namespace {

constexpr bool MulOverflow(uint64_t a, uint64_t b, uint64_t& out) {
    if (a == 0 || b == 0) {
        out = 0;
        return false;
    }
    if (a > std::numeric_limits<uint64_t>::max() / b) {
        return true;
    }
    out = a * b;
    return false;
}

constexpr uint64_t DivRoundBankers(uint64_t value, uint64_t divisor) {
    const uint64_t quotient = value / divisor;
    const uint64_t remainder = value % divisor;
    const uint64_t doubled = remainder * 2;

    if (doubled < divisor) {
        return quotient;
    }
    if (doubled > divisor) {
        return quotient + 1;
    }
    return (quotient % 2 == 0) ? quotient : quotient + 1;
}

std::string ToHex64(uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}

constexpr uint64_t Fnv1a64(const char* data, size_t size) {
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(data[i]));
        hash *= 1099511628211ULL;
    }
    return hash;
}

}  // namespace

std::optional<uint64_t> ConvertTalToDr(uint64_t tal_raw) {
    uint64_t out = 0;
    if (MulOverflow(tal_raw, RATIO_DR_PER_TAL, out)) {
        return std::nullopt;
    }
    return out;
}

std::optional<uint64_t> ConvertDrToOb(uint64_t dr_raw) {
    uint64_t out = 0;
    if (MulOverflow(dr_raw, RATIO_OB_PER_DR, out)) {
        return std::nullopt;
    }
    return out;
}

std::optional<uint64_t> ConvertTalToOb(uint64_t tal_raw) {
    uint64_t out = 0;
    if (MulOverflow(tal_raw, RATIO_OB_PER_TAL, out)) {
        return std::nullopt;
    }
    return out;
}

std::optional<uint64_t> ConvertDrToTal(uint64_t dr_raw, RoundingMode rounding) {
    if (rounding == RoundingMode::Bankers) {
        return DivRoundBankers(dr_raw, RATIO_DR_PER_TAL);
    }
    return dr_raw / RATIO_DR_PER_TAL;
}

std::optional<uint64_t> ConvertObToDr(uint64_t ob_raw, RoundingMode rounding) {
    if (rounding == RoundingMode::Bankers) {
        return DivRoundBankers(ob_raw, RATIO_OB_PER_DR);
    }
    return ob_raw / RATIO_OB_PER_DR;
}

std::optional<uint64_t> ConvertObToTal(uint64_t ob_raw, RoundingMode rounding) {
    if (rounding == RoundingMode::Bankers) {
        return DivRoundBankers(ob_raw, RATIO_OB_PER_TAL);
    }
    return ob_raw / RATIO_OB_PER_TAL;
}

std::string FormatAmount(uint64_t amount_raw, parthenon::primitives::AssetID asset) {
    const uint64_t base = GetTokenBaseUnit(asset);
    const uint64_t whole = amount_raw / base;
    const uint64_t frac = amount_raw % base;
    const uint32_t decimals = GetTokenDecimals(asset);

    std::ostringstream out;
    out << whole << '.' << std::setw(static_cast<int>(decimals)) << std::setfill('0') << frac;
    return out.str();
}

std::optional<uint64_t> ParseDisplayAmount(const std::string& value,
                                           parthenon::primitives::AssetID asset) {
    if (value.empty()) {
        return std::nullopt;
    }

    const uint64_t base = GetTokenBaseUnit(asset);
    const uint32_t decimals = GetTokenDecimals(asset);

    const auto dot = value.find('.');
    const std::string whole_part = dot == std::string::npos ? value : value.substr(0, dot);
    std::string frac_part = dot == std::string::npos ? "" : value.substr(dot + 1);

    if (whole_part.empty() || whole_part[0] == '-') {
        return std::nullopt;
    }

    uint64_t whole = 0;
    auto [ptr, ec] = std::from_chars(whole_part.data(), whole_part.data() + whole_part.size(), whole);
    if (ec != std::errc{} || ptr != whole_part.data() + whole_part.size()) {
        return std::nullopt;
    }

    if (frac_part.size() > decimals) {
        return std::nullopt;
    }

    if (!std::all_of(frac_part.begin(), frac_part.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        return std::nullopt;
    }

    while (frac_part.size() < decimals) {
        frac_part.push_back('0');
    }

    uint64_t frac = 0;
    if (!frac_part.empty()) {
        auto [fptr, fec] = std::from_chars(frac_part.data(), frac_part.data() + frac_part.size(), frac);
        if (fec != std::errc{} || fptr != frac_part.data() + frac_part.size()) {
            return std::nullopt;
        }
    }

    if (whole > std::numeric_limits<uint64_t>::max() / base) {
        return std::nullopt;
    }

    uint64_t raw = whole * base;
    if (raw > std::numeric_limits<uint64_t>::max() - frac) {
        return std::nullopt;
    }

    return raw + frac;
}

AmountView BuildAmountView(uint64_t amount_raw, parthenon::primitives::AssetID asset) {
    return {amount_raw,
            FormatAmount(amount_raw, asset),
            parthenon::primitives::AssetSupply::GetAssetName(asset)};
}

std::string MonetarySpecPayload() {
    std::ostringstream out;
    out << "TAL_DECIMALS=" << TAL_DECIMALS << ';'
        << "DR_DECIMALS=" << DR_DECIMALS << ';'
        << "OB_DECIMALS=" << OB_DECIMALS << ';'
        << "RATIO_DR_PER_TAL=" << RATIO_DR_PER_TAL << ';'
        << "RATIO_OB_PER_DR=" << RATIO_OB_PER_DR << ';'
        << "RATIO_OB_PER_TAL=" << RATIO_OB_PER_TAL;
    return out.str();
}

std::string MonetarySpecHash() {
    const auto payload = MonetarySpecPayload();
    const uint64_t hash = Fnv1a64(payload.data(), payload.size());
    return ToHex64(hash);
}

bool ValidateMonetaryInvariants() {
    if (RATIO_DR_PER_TAL * RATIO_OB_PER_DR != RATIO_OB_PER_TAL) {
        return false;
    }

    if (TAL_BASE_UNIT != DR_BASE_UNIT || DR_BASE_UNIT != OB_BASE_UNIT) {
        return false;
    }

    return TAL_BASE_UNIT == parthenon::primitives::AssetSupply::BASE_UNIT;
}

}  // namespace parthenon::common::monetary
