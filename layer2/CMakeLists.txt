cmake_minimum_required(VERSION 3.10)

add_library(layer2
    bridges/channels/payment_channel.cpp
    bridges/htlc/htlc.cpp
    bridges/spv/spv_bridge.cpp
    indexers/tx_indexer/tx_indexer.cpp
    indexers/contract_indexer/contract_indexer.cpp
    apis/graphql/graphql_api.cpp
    apis/websocket/websocket_api.cpp
    dex/dex.cpp
)

target_include_directories(layer2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../layer1/core
)

target_link_libraries(layer2
    parthenon_crypto
    parthenon_primitives
)

# All layer2 code now organized under proper subdirectories:
# - bridges/channels/ for payment channels
# - bridges/htlc/ for hash time-locked contracts
# - bridges/spv/ for simplified payment verification
# - indexers/ for transaction and contract indexing
# - apis/ for GraphQL and WebSocket APIs
