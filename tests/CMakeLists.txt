# Tests directory

add_subdirectory(unit)

# Integration tests
add_executable(test_integration integration/test_integration.cpp)
target_link_libraries(test_integration
    parthenon_consensus
    parthenon_primitives
    parthenon_chainstate
    parthenon_mempool
    parthenon_mining
    parthenon_wallet
)
add_test(NAME integration COMMAND test_integration)

# Consensus tests
add_executable(test_consensus consensus/test_consensus.cpp)
target_link_libraries(test_consensus
    parthenon_consensus
    parthenon_primitives
)
add_test(NAME consensus COMMAND test_consensus)
