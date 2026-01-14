# Wallet module

add_library(parthenon_wallet STATIC
    wallet.cpp
    hardware/hardware_wallet.cpp
    hardware/firmware_verification.cpp
)

target_include_directories(parthenon_wallet PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/layer1
)

target_link_libraries(parthenon_wallet PUBLIC
    parthenon_crypto
    parthenon_primitives
    parthenon_chainstate
)
