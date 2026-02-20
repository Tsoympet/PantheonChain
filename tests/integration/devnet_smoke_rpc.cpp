#include "rpc/rpc_server.h"

#include <cassert>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    parthenon::rpc::RPCServer l1(0);
    parthenon::rpc::RPCServer l2(0);
    parthenon::rpc::RPCServer l3(0);

    auto spec1 = l1.HandleRequest({"chain/monetary_spec", "[]", "1"});
    auto spec2 = l2.HandleRequest({"chain/monetary_spec", "[]", "2"});
    auto spec3 = l3.HandleRequest({"chain/monetary_spec", "[]", "3"});

    assert(!spec1.IsError());
    assert(!spec2.IsError());
    assert(!spec3.IsError());

    const auto j1 = json::parse(spec1.result);
    const auto j2 = json::parse(spec2.result);
    const auto j3 = json::parse(spec3.result);
    assert(j1["spec_hash"].get<std::string>() == j2["spec_hash"].get<std::string>());
    assert(j2["spec_hash"].get<std::string>() == j3["spec_hash"].get<std::string>());
    assert(j1.contains("denominations"));

    auto stake = l2.HandleRequest({"staking/deposit", "[\"l2\"]", "4"});
    assert(!stake.IsError());
    auto stake_json = json::parse(stake.result);
    assert(stake_json["fee_token"].get<std::string>() == "DRACHMA");

    auto evm = l3.HandleRequest({"evm/deploy", "[\"l3\"]", "5"});
    assert(!evm.IsError());
    auto evm_json = json::parse(evm.result);
    assert(evm_json["fee_token"].get<std::string>() == "OBOLOS");

    std::cout << "rpc smoke passed" << std::endl;
    return 0;
}
