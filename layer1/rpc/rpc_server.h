// ParthenonChain - JSON-RPC Server
// RPC interface for daemon communication

#pragma once

#include "rate_limiter.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

// Forward declarations
namespace parthenon {
namespace node {
class Node;
}
namespace wallet {
class Wallet;
}
}  // namespace parthenon

namespace parthenon {
namespace rpc {

/**
 * JSON-RPC request structure
 */
struct RPCRequest {
    std::string method;
    std::string params;  // JSON string
    std::string id;
};

/**
 * JSON-RPC response structure
 */
struct RPCResponse {
    std::string result;  // JSON string
    std::string error;
    std::string id;

    bool IsError() const { return !error.empty(); }
};

/**
 * RPC method handler function type
 */
using RPCHandler = std::function<RPCResponse(const RPCRequest&)>;

/**
 * JSON-RPC Server
 *
 * Provides HTTP/JSON-RPC interface for blockchain operations.
 * Handles wallet commands, blockchain queries, and network information.
 */
class RPCServer {
  public:
    RPCServer(uint16_t port = 8332);
    ~RPCServer();

    /**
     * Set node instance for blockchain queries
     */
    void SetNode(node::Node* node);

    /**
     * Set wallet instance for wallet operations
     */
    void SetWallet(wallet::Wallet* wallet);

    /**
     * Start the RPC server
     * @return true if server started successfully
     */
    bool Start();

    /**
     * Stop the RPC server
     */
    void Stop();

    /**
     * Check if server is running
     */
    bool IsRunning() const { return running_; }

    /**
     * Register an RPC method handler
     * @param method Method name (e.g., "getinfo", "getbalance")
     * @param handler Function to handle the method
     */
    void RegisterMethod(const std::string& method, RPCHandler handler);

    /**
     * Handle an RPC request
     * @param request JSON-RPC request
     * @param client_ip Client IP address for rate limiting
     * @return JSON-RPC response
     */
    RPCResponse HandleRequest(const RPCRequest& request, const std::string& client_ip = "");

    /**
     * Configure rate limiting
     * @param requests_per_window Maximum requests per window
     * @param window_seconds Window duration in seconds
     */
    void ConfigureRateLimit(uint32_t requests_per_window, uint32_t window_seconds);

  private:
    uint16_t port_;
    bool running_;
    std::map<std::string, RPCHandler> methods_;

    // Component references
    node::Node* node_;
    wallet::Wallet* wallet_;
    
    // Rate limiting
    std::unique_ptr<RateLimiter> rate_limiter_;

    // Initialize standard RPC methods
    void InitializeStandardMethods();

    // Standard method handlers
    RPCResponse HandleGetInfo(const RPCRequest& req);
    RPCResponse HandleGetBalance(const RPCRequest& req);
    RPCResponse HandleGetBlockCount(const RPCRequest& req);
    RPCResponse HandleGetBlock(const RPCRequest& req);
    RPCResponse HandleSendTransaction(const RPCRequest& req);
    RPCResponse HandleGetNewAddress(const RPCRequest& req);
    RPCResponse HandleSendToAddress(const RPCRequest& req);
    RPCResponse HandleStop(const RPCRequest& req);
};

}  // namespace rpc
}  // namespace parthenon
