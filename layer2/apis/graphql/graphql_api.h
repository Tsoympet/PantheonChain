// ParthenonChain - GraphQL API
// GraphQL endpoint for blockchain queries

#pragma once

#include <functional>
#include <memory>
#include <string>

namespace parthenon {
namespace layer2 {
namespace apis {

/**
 * GraphQL API Server
 *
 * Provides GraphQL endpoint for flexible blockchain queries
 */
class GraphQLAPI {
  public:
    GraphQLAPI(uint16_t port = 8080);
    ~GraphQLAPI();

    /**
     * Start the GraphQL server
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
     * Handle GraphQL query
     */
    std::string HandleQuery(const std::string& query);

    /**
     * Set callback for block queries
     */
    void SetBlockCallback(std::function<std::string(const std::string&)> callback);

    /**
     * Set callback for transaction queries
     */
    void SetTransactionCallback(std::function<std::string(const std::string&)> callback);

    /**
     * Set callback for contract queries
     */
    void SetContractCallback(std::function<std::string(const std::string&)> callback);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace apis
}  // namespace layer2
}  // namespace parthenon
