// ParthenonChain - WebSocket API
// WebSocket endpoint for real-time blockchain updates

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <cstdint>

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
     * Check if server is running
     */
    bool IsRunning() const;

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

    /**
     * Subscribe client to a topic
     */
    void Subscribe(uint64_t client_id, const std::string& topic);

    /**
     * Unsubscribe client from a topic
     */
    void Unsubscribe(uint64_t client_id, const std::string& topic);

    /**
     * Publish message to specific topic
     */
    void PublishToTopic(const std::string& topic, const std::string& message);

    /**
     * Get number of connected clients
     */
    size_t GetConnectedClients() const;

    /**
     * Notify about new block
     */
    void NotifyNewBlock(const std::string& block_data);

    /**
     * Notify about new transaction
     */
    void NotifyNewTransaction(const std::string& tx_data);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace apis
}  // namespace layer2
}  // namespace parthenon
