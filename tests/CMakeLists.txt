# Tests directory

# Suppress unused variable warnings in test files
if(MSVC)
  add_compile_options(/wd4101 /wd4189)
else()
  add_compile_options(-Wno-unused-variable -Wno-unused-but-set-variable)
endif()

add_subdirectory(unit)
add_subdirectory(fuzzing)

# Integration tests
add_executable(test_integration integration/test_integration.cpp)
target_link_libraries(test_integration
    parthenon_consensus
    parthenon_primitives
    parthenon_chainstate
    parthenon_mempool
    parthenon_mining
    parthenon_wallet
    parthenon_p2p
    parthenon_evm
)
add_test(NAME integration COMMAND test_integration)

# Automated integration tests (comprehensive)
add_executable(test_integration_automated integration/test_integration_automated.cpp)
target_link_libraries(test_integration_automated
    parthenon_node
    parthenon_consensus
    parthenon_primitives
    parthenon_chainstate
    parthenon_mempool
    parthenon_mining
    parthenon_wallet
    parthenon_p2p
    parthenon_storage
)
add_test(NAME integration_automated COMMAND test_integration_automated)

# Mobile SDK regtest integration (POSIX sockets – not supported on Windows)
if(NOT WIN32)
add_executable(test_mobile_sdk_regtest integration/test_mobile_sdk_regtest.cpp)
target_link_libraries(test_mobile_sdk_regtest
    parthenon_mobile_sdk
    parthenon_crypto
)
add_test(NAME mobile_sdk_regtest COMMAND test_mobile_sdk_regtest)
endif()

# Consensus tests
add_executable(test_consensus consensus/test_consensus.cpp)
target_link_libraries(test_consensus
    parthenon_consensus
    parthenon_primitives
)
add_test(NAME consensus COMMAND test_consensus)

# Devnet smoke RPC integration (headless)
add_executable(test_devnet_smoke_rpc integration/devnet_smoke_rpc.cpp)
target_link_libraries(test_devnet_smoke_rpc
    parthenon_rpc
)
add_test(NAME devnet_smoke_rpc COMMAND test_devnet_smoke_rpc)
