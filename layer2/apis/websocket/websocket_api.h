// ParthenonChain - WebSocket API
// WebSocket endpoint for real-time blockchain updates

#pragma once

#include <string>
#include <functional>

namespace parthenon {
namespace layer2 {
namespace apis {

/**
 * WebSocket API Server
 * 
 * Provides WebSocket endpoint for real-time blockchain updates
 */
class WebSocketAPI {
public:
    WebSocketAPI(uint16_t port = 8081);
    ~WebSocketAPI();
    
    /**
     * Start the WebSocket server
     */
    bool Start();
    
    /**
     * Stop the server
     */
    void Stop();
    
    /**
     * Broadcast message to all connected clients
     */
    void Broadcast(const std::string& message);
    
    /**
     * Subscribe callback for new blocks
     */
    void OnNewBlock(std::function<void(const std::string&)> callback);
    
    /**
     * Subscribe callback for new transactions
     */
    void OnNewTransaction(std::function<void(const std::string&)> callback);
    
private:
    uint16_t port_;
    bool running_;
    
    // TODO: Implement WebSocket server (using libwebsockets or similar)
    // TODO: Add authentication/rate limiting
    // TODO: Implement subscription management
};

} // namespace apis
} // namespace layer2
} // namespace parthenon
