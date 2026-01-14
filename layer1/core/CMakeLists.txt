# Layer 1 Core - Cryptography and Primitives

# Crypto library
add_library(parthenon_crypto STATIC
    crypto/sha256.cpp
    crypto/schnorr.cpp
    crypto/hardware_crypto.cpp
    crypto/post_quantum/pq_crypto.cpp
)

target_include_directories(parthenon_crypto PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/secp256k1/include
)

target_link_libraries(parthenon_crypto PUBLIC
    secp256k1
)

# Set strict compiler flags for consensus code
if(MSVC)
  target_compile_options(parthenon_crypto PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_crypto PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Privacy library (zkSNARK and ring signatures)
add_library(parthenon_privacy STATIC
    privacy/zk_snark.cpp
    privacy/ring_signature.cpp
)

target_include_directories(parthenon_privacy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_privacy PUBLIC
    parthenon_crypto
)

# Set strict compiler flags
if(MSVC)
  target_compile_options(parthenon_privacy PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_privacy PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Sharding library (cross-shard transactions)
add_library(parthenon_sharding STATIC
    sharding/shard.cpp
)

target_include_directories(parthenon_sharding PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_sharding PUBLIC
    parthenon_crypto
    parthenon_primitives
)

# Set strict compiler flags
if(MSVC)
  target_compile_options(parthenon_sharding PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_sharding PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Governance library
add_library(parthenon_governance STATIC
    governance/governance.cpp
)

target_include_directories(parthenon_governance PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_governance PUBLIC
    parthenon_crypto
)

# Set strict compiler flags
if(MSVC)
  target_compile_options(parthenon_governance PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_governance PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Consensus library (depends on crypto)
add_library(parthenon_consensus STATIC
    consensus/difficulty.cpp
    consensus/issuance.cpp
)

target_include_directories(parthenon_consensus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_consensus PUBLIC
    parthenon_crypto
)

# Set strict compiler flags for consensus code
if(MSVC)
  target_compile_options(parthenon_consensus PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_consensus PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Primitives library
add_library(parthenon_primitives STATIC
    primitives/amount.cpp
    primitives/asset.cpp
    primitives/transaction.cpp
    primitives/block.cpp
)

target_include_directories(parthenon_primitives PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_primitives PUBLIC
    parthenon_crypto
    parthenon_consensus
)

# Set strict compiler flags for consensus code
if(MSVC)
  target_compile_options(parthenon_primitives PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_primitives PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Chainstate library
add_library(parthenon_chainstate STATIC
    chainstate/chainstate.cpp
    chainstate/utxo.cpp
    chainstate/chain.cpp
)

target_include_directories(parthenon_chainstate PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_chainstate PUBLIC
    parthenon_primitives
    parthenon_consensus
)

# Set strict compiler flags for consensus code
if(MSVC)
  target_compile_options(parthenon_chainstate PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_chainstate PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Validation library
add_library(parthenon_validation STATIC
    validation/validation.cpp
)

target_include_directories(parthenon_validation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_validation PUBLIC
    parthenon_chainstate
    parthenon_primitives
    parthenon_consensus
)

# Set strict compiler flags for consensus code
if(MSVC)
  target_compile_options(parthenon_validation PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_validation PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# P2P library
add_library(parthenon_p2p STATIC
    p2p/protocol.cpp
    p2p/message.cpp
    p2p/peer_database.cpp
    p2p/peer_discovery.cpp
)

target_include_directories(parthenon_p2p PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_p2p PUBLIC
    parthenon_primitives
    parthenon_crypto
)

# Set strict compiler flags
if(MSVC)
  target_compile_options(parthenon_p2p PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_p2p PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Mempool library
add_library(parthenon_mempool STATIC
    mempool/mempool.cpp
)

target_include_directories(parthenon_mempool PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_mempool PUBLIC
    parthenon_chainstate
    parthenon_validation
    parthenon_primitives
)

# Set strict compiler flags
if(MSVC)
  target_compile_options(parthenon_mempool PRIVATE /W4 /WX)
else()
  target_compile_options(parthenon_mempool PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
  )
endif()

# Mining library
add_subdirectory(mining)

# Storage library
add_subdirectory(storage)

# Node library
add_subdirectory(node)
