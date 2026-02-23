// ParthenonChain - RPC Server Tests
// Validate RPC request wiring for daemon control

#include "node/node.h"
#include "rpc/rpc_server.h"
#include "rpc/validation.h"
#include "wallet/wallet.h"

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
    assert(server.IsAuthorized("Basic   cnBjdXNlcjpycGNwYXNz   "));
    assert(!server.IsAuthorized("Basic invalid"));
    assert(!server.IsAuthorized("Basic cnBjdXNlcjpycGNwYXN6"));
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

void TestSendRawTransactionRejectsInvalidHex() {
    std::cout << "Test: sendrawtransaction rejects invalid hex" << std::endl;

    const auto unique_suffix =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto temp_dir =
        std::filesystem::temp_directory_path() / ("parthenon-rpc-test-raw-" + unique_suffix);

    node::Node node(temp_dir.string(), 0);
    rpc::RPCServer server;
    server.SetNode(&node);

    rpc::RPCRequest request;
    request.method = "sendrawtransaction";
    request.id = "3";
    request.params = R"(["zz11"] )";

    auto response = server.HandleRequest(request);
    assert(response.IsError());
    assert(!response.error.empty());

    std::error_code cleanup_error;
    std::filesystem::remove_all(temp_dir, cleanup_error);

    std::cout << "  ✓ Passed (invalid hex)" << std::endl;
}


void TestValidationParsingAndSanitization() {
    std::cout << "Test: input validator parsing/sanitization" << std::endl;

    using parthenon::rpc::InputValidator;

    // ParseUint64 strict-decimal behavior
    assert(InputValidator::ParseUint64("0").has_value());
    assert(InputValidator::ParseUint64("18446744073709551615").has_value());
    assert(!InputValidator::ParseUint64("18446744073709551616").has_value());
    assert(!InputValidator::ParseUint64("1abc").has_value());
    assert(!InputValidator::ParseUint64("+1").has_value());

    // SanitizeString should keep safe chars and strip control/non-ASCII bytes
    const std::string dangerous = std::string("Asset-01_ ") + "\x01" + "\xff" + "<>";
    const std::string sanitized = InputValidator::SanitizeString(dangerous);
    assert(sanitized == "Asset-01_ ");

    std::cout << "  ✓ Passed (validator strict parsing)" << std::endl;
}

void TestMonetarySpecEndpoint() {
    std::cout << "Test: monetary spec endpoint" << std::endl;

    rpc::RPCServer server;
    rpc::RPCRequest request;
    request.method = "chain/monetary_spec";
    request.id = "7";
    request.params = "[]";

    auto response = server.HandleRequest(request);
    assert(!response.IsError());
    assert(response.result.find("spec_hash") != std::string::npos);

    std::cout << "  ✓ Passed (monetary spec)" << std::endl;
}

void TestSendToAddressRejectsInvalidAmountAndHex() {
    std::cout << "Test: sendtoaddress rejects invalid amount/address" << std::endl;

    const auto unique_suffix =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto temp_dir =
        std::filesystem::temp_directory_path() / ("parthenon-rpc-test-sendto-" + unique_suffix);

    node::Node node(temp_dir.string(), 0);
    std::array<uint8_t, 32> seed{};
    wallet::Wallet wallet(seed);

    rpc::RPCServer server;
    server.SetNode(&node);
    server.SetWallet(&wallet);

    rpc::RPCRequest bad_amount_request;
    bad_amount_request.method = "sendtoaddress";
    bad_amount_request.id = "4";
    bad_amount_request.params = R"(["0011", "not-a-number"])";

    auto bad_amount_response = server.HandleRequest(bad_amount_request);
    assert(bad_amount_response.IsError());
    assert(!bad_amount_response.error.empty());

    rpc::RPCRequest bad_address_request;
    bad_address_request.method = "sendtoaddress";
    bad_address_request.id = "5";
    bad_address_request.params = R"(["nothex", "1"])";

    auto bad_address_response = server.HandleRequest(bad_address_request);
    assert(bad_address_response.IsError());
    assert(!bad_address_response.error.empty());

    rpc::RPCRequest bad_asset_request;
    bad_asset_request.method = "sendtoaddress";
    bad_asset_request.id = "6";
    bad_asset_request.params = R"(["0011", "1", 99])";

    auto bad_asset_response = server.HandleRequest(bad_asset_request);
    assert(bad_asset_response.IsError());
    assert(!bad_asset_response.error.empty());

    std::error_code cleanup_error;
    std::filesystem::remove_all(temp_dir, cleanup_error);

    std::cout << "  ✓ Passed (invalid amount/address)" << std::endl;
}

int main() {
    std::cout << "=== RPC Server Tests ===" << std::endl;

    TestStopMethodWithoutNode();
    TestStopMethodWithNode();
    TestBasicAuthConfiguration();
    TestServerStartStopLifecycle();
    TestSendRawTransactionRejectsInvalidHex();
    TestSendToAddressRejectsInvalidAmountAndHex();
    TestValidationParsingAndSanitization();
    TestMonetarySpecEndpoint();

    std::cout << "✓ All RPC server tests passed!" << std::endl;
    return 0;
}
