#include "common/monetary/units.h"
#include "common/monetary/denominations.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
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

std::string Trim(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char c) {
                    return !std::isspace(c);
                }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char c) {
                    return !std::isspace(c);
                }).base(),
                value.end());
    return value;
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::optional<uint64_t> Pow10(uint32_t exponent) {
    uint64_t value = 1;
    for (uint32_t i = 0; i < exponent; ++i) {
        if (value > std::numeric_limits<uint64_t>::max() / 10ULL) {
            return std::nullopt;
        }
        value *= 10ULL;
    }
    return value;
}

std::optional<uint64_t> ParseScaledDecimal(const std::string& value, uint32_t decimals) {
    if (value.empty()) {
        return std::nullopt;
    }

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

    const auto scale = Pow10(decimals);
    if (!scale || whole > std::numeric_limits<uint64_t>::max() / *scale) {
        return std::nullopt;
    }

    const uint64_t raw = whole * (*scale);
    if (raw > std::numeric_limits<uint64_t>::max() - frac) {
        return std::nullopt;
    }

    return raw + frac;
}

std::string FormatScaledDecimal(uint64_t scaled, uint32_t decimals) {
    const auto scale = Pow10(decimals).value_or(1);
    const uint64_t whole = scale == 0 ? scaled : scaled / scale;
    const uint64_t frac = scale == 0 ? 0 : scaled % scale;

    std::ostringstream out;
    out << whole;
    if (decimals > 0) {
        out << '.' << std::setw(static_cast<int>(decimals)) << std::setfill('0') << frac;
    }
    return out.str();
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

std::optional<std::string> FormatAmountWithDenomination(uint64_t amount_raw,
                                                        parthenon::primitives::AssetID asset,
                                                        const std::string& denomination,
                                                        bool* approximate) {
    const auto* denom = ResolveDenomination(asset, denomination.empty() ? DefaultDenominationName(asset)
                                                                        : denomination);
    if (!denom) {
        return std::nullopt;
    }

    const uint64_t scale = Pow10(denom->allowed_decimals).value_or(1);
    uint64_t lhs = 0;
    if (MulOverflow(amount_raw, denom->token_units_denominator, lhs)) {
        return std::nullopt;
    }

    uint64_t numer = 0;
    if (MulOverflow(lhs, scale, numer)) {
        return std::nullopt;
    }

    const uint64_t base = GetTokenBaseUnit(asset);
    uint64_t denom_divisor = 0;
    if (MulOverflow(base, denom->token_units_numerator, denom_divisor) || denom_divisor == 0) {
        return std::nullopt;
    }

    const bool is_approximate = (numer % denom_divisor) != 0;
    if (approximate != nullptr) {
        *approximate = is_approximate || denom->approximate_display;
    }

    const uint64_t scaled_amount = numer / denom_divisor;
    return FormatScaledDecimal(scaled_amount, denom->allowed_decimals);
}

std::optional<uint64_t> ParseDisplayAmount(const std::string& value,
                                           parthenon::primitives::AssetID asset) {
    return ParseScaledDecimal(value, GetTokenDecimals(asset));
}

std::optional<uint64_t> ParseDisplayAmountWithDenomination(const std::string& value,
                                                           parthenon::primitives::AssetID asset,
                                                           const std::string& denomination,
                                                           std::string* error) {
    const auto* denom = ResolveDenomination(asset, denomination.empty() ? DefaultDenominationName(asset)
                                                                        : denomination);
    if (!denom) {
        if (error) {
            *error = "unknown denomination";
        }
        return std::nullopt;
    }

    if (!denom->allow_input) {
        if (error) {
            *error = "denomination is display-only";
        }
        return std::nullopt;
    }

    const auto scaled = ParseScaledDecimal(value, denom->allowed_decimals);
    if (!scaled) {
        if (error) {
            *error = "invalid numeric amount";
        }
        return std::nullopt;
    }

    const uint64_t scale = Pow10(denom->allowed_decimals).value_or(1);
    uint64_t denom_raw = 0;
    if (MulOverflow(*scaled, denom->token_units_numerator, denom_raw)) {
        if (error) {
            *error = "amount overflow";
        }
        return std::nullopt;
    }

    const uint64_t base = GetTokenBaseUnit(asset);
    uint64_t numer = 0;
    if (MulOverflow(denom_raw, base, numer)) {
        if (error) {
            *error = "amount overflow";
        }
        return std::nullopt;
    }

    uint64_t denom_divisor = 0;
    if (MulOverflow(denom->token_units_denominator, scale, denom_divisor) || denom_divisor == 0) {
        if (error) {
            *error = "invalid denomination divisor";
        }
        return std::nullopt;
    }

    if ((numer % denom_divisor) != 0) {
        if (error) {
            *error = "denomination not exactly representable with current decimals";
        }
        return std::nullopt;
    }

    return numer / denom_divisor;
}

AmountView BuildAmountView(uint64_t amount_raw, parthenon::primitives::AssetID asset,
                           const std::string& denomination, bool dual_display) {
    const std::string denom_name = denomination.empty() ? DefaultDenominationName(asset) : Lower(Trim(denomination));
    bool approximate = false;
    const auto formatted = FormatAmountWithDenomination(amount_raw, asset, denom_name, &approximate);
    const auto canonical = FormatAmount(amount_raw, asset);

    AmountView view{amount_raw,
                    formatted.value_or(canonical),
                    formatted.value_or(canonical),
                    parthenon::primitives::AssetSupply::GetAssetName(asset),
                    denom_name,
                    std::nullopt,
                    approximate};

    if (dual_display) {
        if (asset == parthenon::primitives::AssetID::DRACHMA) {
            const auto in_ob = ConvertDrToOb(amount_raw);
            if (in_ob) {
                view.dual_display = view.amount_formatted + " DR (" + FormatAmount(*in_ob, parthenon::primitives::AssetID::OBOLOS) + " OB)";
            }
        } else if (asset == parthenon::primitives::AssetID::TALANTON) {
            const auto in_dr = ConvertTalToDr(amount_raw);
            if (in_dr) {
                view.dual_display = view.amount_formatted + " TAL (" + FormatAmount(*in_dr, parthenon::primitives::AssetID::DRACHMA) + " DR)";
            }
        }
    }

    return view;
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
