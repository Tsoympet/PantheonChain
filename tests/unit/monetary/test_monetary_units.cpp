#include "common/monetary/units.h"
#include "rpc/rpc_server.h"

#include <cassert>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    using namespace parthenon::common::monetary;

    assert(ValidateMonetaryInvariants());

    const uint64_t one_tal = TAL_BASE_UNIT;
    const uint64_t one_dr = DR_BASE_UNIT;

    assert(ConvertTalToDr(one_tal).value() == 6000ULL * one_dr);
    assert(ConvertDrToOb(one_dr).value() == 6ULL * OB_BASE_UNIT);
    assert(ConvertTalToOb(one_tal).value() == 36000ULL * OB_BASE_UNIT);

    assert(ConvertObToDr(7, RoundingMode::Floor).value() == 1);
    assert(ConvertObToDr(9, RoundingMode::Bankers).value() == 2);
    assert(ConvertObToDr(15, RoundingMode::Bankers).value() == 2);

    const auto parsed = ParseDisplayAmount("1.23456789", parthenon::primitives::AssetID::DRACHMA);
    assert(parsed.has_value());
    assert(FormatAmount(*parsed, parthenon::primitives::AssetID::DRACHMA) == "1.23456789");

    assert(ParseDisplayAmountWithDenomination("2", parthenon::primitives::AssetID::DRACHMA,
                                              "tetradrachm")
               .value() == 8ULL * DR_BASE_UNIT);
    assert(ParseDisplayAmountWithDenomination("3", parthenon::primitives::AssetID::DRACHMA, "mina")
               .value() == 300ULL * DR_BASE_UNIT);
    std::string err;
    assert(!ParseDisplayAmountWithDenomination("1", parthenon::primitives::AssetID::OBOLOS,
                                               "hemiobol", &err)
                .has_value());
    assert(err == "denomination is display-only");

    bool approx = false;
    auto in_obol = FormatAmountWithDenomination(
        2ULL * DR_BASE_UNIT, parthenon::primitives::AssetID::DRACHMA, "obol", &approx);
    assert(in_obol.has_value());
    assert(*in_obol == "12.00000000");

    const auto dual = BuildAmountView(2ULL * DR_BASE_UNIT, parthenon::primitives::AssetID::DRACHMA,
                                      "drachma", true);
    assert(dual.dual_display.has_value());

    parthenon::rpc::RPCServer rpc(0);
    auto spec = rpc.HandleRequest({"chain/monetary_spec", "[]", "1"});
    assert(!spec.IsError());
    auto j = json::parse(spec.result);
    assert(j["spec_hash"].get<std::string>() == MonetarySpecHash());
    assert(j.contains("denominations"));

    std::cout << "monetary unit tests passed" << std::endl;
    return 0;
}
