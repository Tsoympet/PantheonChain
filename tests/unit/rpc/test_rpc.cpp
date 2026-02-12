// ParthenonChain - RPC Server Tests
// Validate RPC request wiring for daemon control

#include "node/node.h"
#include "rpc/rpc_server.h"

#include <cassert>
#include <iostream>

using namespace parthenon;

void TestStopMethodWithoutNode() {
    std::cout << "Test: stop method without node" << std::endl;

    rpc::RPCServer server;
    rpc::RPCRequest request;
    request.method = "stop";
    request.id = "1";

    auto response = server.HandleRequest(request);
    assert(response.IsError());
    assert(response.error == "Node not initialized");

    std::cout << "  ✓ Passed (missing node)" << std::endl;
}

void TestStopMethodWithNode() {
    std::cout << "Test: stop method with node" << std::endl;

    node::Node node("/tmp/parthenon-rpc-test", 0);
    rpc::RPCServer server;
    server.SetNode(&node);

    rpc::RPCRequest request;
    request.method = "stop";
    request.id = "2";

    auto response = server.HandleRequest(request);
    assert(!response.IsError());
    assert(response.result == "\"Node stopping\"");

    std::cout << "  ✓ Passed (stop response)" << std::endl;
}

int main() {
    std::cout << "=== RPC Server Tests ===" << std::endl;

    TestStopMethodWithoutNode();
    TestStopMethodWithNode();

    std::cout << "✓ All RPC server tests passed!" << std::endl;
    return 0;
}
