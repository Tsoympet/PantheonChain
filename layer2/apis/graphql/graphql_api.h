// ParthenonChain - GraphQL API
// GraphQL endpoint for blockchain queries

#pragma once

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
     * Handle GraphQL query
     */
    std::string HandleQuery(const std::string& query);
    
private:
    uint16_t port_;
    bool running_;
    
    // TODO: Implement GraphQL schema and resolvers
    // TODO: Add authentication/rate limiting
};

} // namespace apis
} // namespace layer2
} // namespace parthenon
