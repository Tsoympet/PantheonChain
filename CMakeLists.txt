cmake_minimum_required(VERSION 3.15)
project(PantheonChain VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(pantheon_common STATIC
  common/crypto/cryptography.cpp
  common/p2p/p2p_network.cpp
  common/storage/storage.cpp
  common/serialization/serialization.cpp
  common/serialization/commitments.cpp
  common/mempool/mempool.cpp
  common/metrics/metrics.cpp
  common/monetary/denominations.cpp
  common/monetary/units.cpp
  common/utils/bridging.cpp
)
target_include_directories(pantheon_common PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/layer1-talanton/core)

add_library(pantheon_l1 STATIC layer1-talanton/tx/l1_commitment_validator.cpp)
target_include_directories(pantheon_l1 PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/layer1-talanton/core)
target_link_libraries(pantheon_l1 PUBLIC pantheon_common)

add_library(pantheon_l2 STATIC
  layer2-drachma/consensus/pos_consensus.cpp
  layer2-drachma/payments/payments_state_machine.cpp
)
target_include_directories(pantheon_l2 PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/layer1-talanton/core)
target_link_libraries(pantheon_l2 PUBLIC pantheon_common)

add_library(pantheon_l3 STATIC
  layer3-obolos/consensus/pos_consensus.cpp
  layer3-obolos/evm/execution.cpp
)
target_include_directories(pantheon_l3 PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/layer1-talanton/core)
target_link_libraries(pantheon_l3 PUBLIC pantheon_common)

add_executable(pantheon-l1-talanton-node common/utils/pantheon_node.cpp)
add_executable(pantheon-l2-drachma-node common/utils/pantheon_node.cpp)
add_executable(pantheon-l3-obolos-node common/utils/pantheon_node.cpp)
target_compile_definitions(pantheon-l1-talanton-node PRIVATE PANTHEON_NODE_LAYER="l1")
target_compile_definitions(pantheon-l2-drachma-node PRIVATE PANTHEON_NODE_LAYER="l2")
target_compile_definitions(pantheon-l3-obolos-node PRIVATE PANTHEON_NODE_LAYER="l3")

add_executable(pantheon-cli cli/pantheon-cli/main.cpp)
target_link_libraries(pantheon-cli PRIVATE pantheon_common)

add_executable(pantheon-relayer-l2 relayers/relayer-l2/main.cpp)
add_executable(pantheon-relayer-l3 relayers/relayer-l3/main.cpp)
target_link_libraries(pantheon-relayer-l2 PRIVATE pantheon_common pantheon_l1 pantheon_l2)
target_link_libraries(pantheon-relayer-l3 PRIVATE pantheon_common pantheon_l2)

# Third-party stubs (secp256k1 and leveldb) used by layer1-talanton
set(SECP256K1_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/stubs/secp256k1/include")
add_subdirectory(third_party/stubs/secp256k1)
add_subdirectory(third_party/stubs/leveldb)

# Compatibility shim: parthenon_monetary wraps the monetary code in pantheon_common
add_library(parthenon_monetary INTERFACE)
target_link_libraries(parthenon_monetary INTERFACE pantheon_common)

# Convenience aggregate: pantheon_layers bundles all layer libraries for tests/relayers
add_library(pantheon_layers INTERFACE)
target_link_libraries(pantheon_layers INTERFACE pantheon_common pantheon_l1 pantheon_l2 pantheon_l3)

# Mobile SDK and other tools
add_subdirectory(tools)

# EVM library (Layer 3 Obolos execution engine)
add_subdirectory(layer3-obolos/evm)

# Layer 2 Drachma library (bridges, indexers, DEX, rollups, etc.)
add_subdirectory(layer2-drachma)

# Layer 1 full library set (parthenon_node, parthenon_wallet, parthenon_rpc, etc.)
add_subdirectory(layer1-talanton)

# Client applications: parthenond (daemon), parthenon-cli, parthenon-qt (desktop wallet)
add_subdirectory(clients)

# Test suite
add_subdirectory(tests)
