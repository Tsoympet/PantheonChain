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

    parthenon::rpc::RPCServer rpc(0);
    auto spec = rpc.HandleRequest({"chain/monetary_spec", "[]", "1"});
    assert(!spec.IsError());
    auto j = json::parse(spec.result);
    assert(j["spec_hash"].get<std::string>() == MonetarySpecHash());

    std::cout << "monetary unit tests passed" << std::endl;
    return 0;
}
