# Consensus tests

add_executable(test_difficulty test_difficulty.cpp)
target_link_libraries(test_difficulty PRIVATE
    parthenon_consensus
    parthenon_crypto
)
add_test(NAME test_difficulty COMMAND test_difficulty)

add_executable(test_issuance test_issuance.cpp)
target_link_libraries(test_issuance PRIVATE
    parthenon_consensus
    parthenon_crypto
)
add_test(NAME test_issuance COMMAND test_issuance)

add_executable(test_supply_caps test_supply_caps.cpp)
target_link_libraries(test_supply_caps PRIVATE
    parthenon_consensus
    parthenon_crypto
    parthenon_primitives
)
add_test(NAME test_supply_caps COMMAND test_supply_caps)

add_executable(test_determinism test_determinism.cpp)
target_link_libraries(test_determinism PRIVATE
    parthenon_consensus
    parthenon_crypto
    parthenon_primitives
)
add_test(NAME test_determinism COMMAND test_determinism)

add_executable(test_genesis test_genesis.cpp)
target_link_libraries(test_genesis PRIVATE
    parthenon_consensus
    parthenon_primitives
)
add_test(NAME test_genesis COMMAND test_genesis)
