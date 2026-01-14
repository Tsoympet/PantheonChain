# Wallet Unit Tests

add_executable(test_hardware_wallet test_hardware_wallet.cpp)

target_link_libraries(test_hardware_wallet
    parthenon_wallet
    parthenon_crypto
    parthenon_primitives
)

add_test(NAME test_hardware_wallet COMMAND test_hardware_wallet)
