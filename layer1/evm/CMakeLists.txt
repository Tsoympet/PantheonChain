# EVM library
add_library(parthenon_evm STATIC
    vm.cpp
    state.cpp
    opcodes.cpp
    mpt.cpp
    formal_verification/verifier.cpp
    private_contracts.cpp
)

target_include_directories(parthenon_evm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

target_link_libraries(parthenon_evm PUBLIC
    parthenon_crypto
    parthenon_primitives
    parthenon_privacy
)
