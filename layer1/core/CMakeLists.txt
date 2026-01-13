# Layer 1 Core - Cryptography and Primitives

# Crypto library
add_library(parthenon_crypto STATIC
    crypto/sha256.cpp
    crypto/schnorr.cpp
)

target_include_directories(parthenon_crypto PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/secp256k1/include
)

target_link_libraries(parthenon_crypto PUBLIC
    secp256k1
)

# Set strict compiler flags for consensus code
target_compile_options(parthenon_crypto PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)

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
target_compile_options(parthenon_consensus PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)

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
target_compile_options(parthenon_primitives PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)

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
target_compile_options(parthenon_chainstate PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)

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
target_compile_options(parthenon_validation PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)

# P2P library
add_library(parthenon_p2p STATIC
    p2p/protocol.cpp
    p2p/message.cpp
)

target_include_directories(parthenon_p2p PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parthenon_p2p PUBLIC
    parthenon_primitives
    parthenon_crypto
)

# Set strict compiler flags
target_compile_options(parthenon_p2p PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)

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
target_compile_options(parthenon_mempool PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)

# Mining library
add_subdirectory(mining)

# Node library
add_subdirectory(node)
