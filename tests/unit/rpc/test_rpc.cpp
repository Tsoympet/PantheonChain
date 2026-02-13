// ParthenonChain - RPC Server Tests
// Validate RPC request wiring for daemon control

#include "node/node.h"
#include "rpc/rpc_server.h"

#include <cassert>
#include <chrono>
#include <filesystem>
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

    const auto unique_suffix =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto temp_dir =
        std::filesystem::temp_directory_path() / ("parthenon-rpc-test-" + unique_suffix);
    node::Node node(temp_dir.string(), 0);
    rpc::RPCServer server;
    server.SetNode(&node);

    rpc::RPCRequest request;
    request.method = "stop";
    request.id = "2";

    auto response = server.HandleRequest(request);
    assert(!response.IsError());
    assert(response.result == "\"Node stopping\"");

    std::error_code cleanup_error;
    std::filesystem::remove_all(temp_dir, cleanup_error);

    std::cout << "  ✓ Passed (stop response)" << std::endl;
}


void TestBasicAuthConfiguration() {
    std::cout << "Test: basic auth configuration" << std::endl;

    rpc::RPCServer server;
    assert(!server.IsAuthenticationEnabled());

    server.ConfigureBasicAuth("rpcuser", "rpcpass");
    assert(server.IsAuthenticationEnabled());

    // base64("rpcuser:rpcpass") = cnBjdXNlcjpycGNwYXNz
    assert(server.IsAuthorized("Basic cnBjdXNlcjpycGNwYXNz"));
    assert(server.IsAuthorized("basic cnBjdXNlcjpycGNwYXNz"));
    assert(!server.IsAuthorized("Basic invalid"));
    assert(!server.IsAuthorized("Bearer token"));

    std::cout << "  ✓ Passed (auth checks)" << std::endl;
}


void TestServerStartStopLifecycle() {
    std::cout << "Test: rpc server start/stop lifecycle" << std::endl;

    rpc::RPCServer server(0);
    assert(server.Start());
    assert(server.IsRunning());

    server.Stop();
    assert(!server.IsRunning());

    // Ensure server can be started again after a clean stop.
    assert(server.Start());
    assert(server.IsRunning());
    server.Stop();
    assert(!server.IsRunning());

    std::cout << "  ✓ Passed (lifecycle)" << std::endl;
}

int main() {
    std::cout << "=== RPC Server Tests ===" << std::endl;

    TestStopMethodWithoutNode();
    TestStopMethodWithNode();
    TestBasicAuthConfiguration();
    TestServerStartStopLifecycle();

    std::cout << "✓ All RPC server tests passed!" << std::endl;
    return 0;
}
