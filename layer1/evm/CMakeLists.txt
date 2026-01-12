# EVM module

add_library(parthenon_evm STATIC
    opcodes.cpp
    state.cpp
    vm.cpp
)

target_include_directories(parthenon_evm PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/layer1
)

target_link_libraries(parthenon_evm PUBLIC
    parthenon_crypto
    parthenon_primitives
)
