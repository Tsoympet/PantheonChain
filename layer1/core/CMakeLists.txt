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
)

# Set strict compiler flags for consensus code
target_compile_options(parthenon_primitives PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)
