#include "rpc/rpc_server.h"

#include <cassert>
#include <iostream>

int main() {
    parthenon::rpc::RPCServer server(0);

    auto info = server.HandleRequest({"chain/info", "[]", "1"});
    assert(!info.IsError());

    auto submit = server.HandleRequest({"commitments/submit", "[{\"layer\":\"l3\"}]", "2"});
    assert(!submit.IsError());

    auto list = server.HandleRequest({"commitments/list", "[]", "3"});
    assert(!list.IsError());
    assert(list.result.find("commitments") != std::string::npos);

    auto stake = server.HandleRequest({"staking/deposit", "[\"l2\"]", "4"});
    assert(!stake.IsError());

    auto evm = server.HandleRequest({"evm/deploy", "[\"l3\"]", "5"});
    assert(!evm.IsError());

    std::cout << "rpc smoke passed" << std::endl;
    return 0;
}
