// ParthenonChain - JSON-RPC Server
// RPC interface for daemon communication

#pragma once

#include <string>
#include <functional>
#include <map>
#include <memory>

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
     * @return JSON-RPC response
     */
    RPCResponse HandleRequest(const RPCRequest& request);
    
private:
    uint16_t port_;
    bool running_;
    std::map<std::string, RPCHandler> methods_;
    
    // Initialize standard RPC methods
    void InitializeStandardMethods();
    
    // Standard method handlers (stubs for now)
    RPCResponse HandleGetInfo(const RPCRequest& req);
    RPCResponse HandleGetBalance(const RPCRequest& req);
    RPCResponse HandleGetBlockCount(const RPCRequest& req);
    RPCResponse HandleGetBlock(const RPCRequest& req);
    RPCResponse HandleSendTransaction(const RPCRequest& req);
};

} // namespace rpc
} // namespace parthenon
