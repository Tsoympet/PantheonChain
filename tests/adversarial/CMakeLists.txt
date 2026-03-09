# Adversarial pre-launch test suite
# All targets link against pantheon_layers (includes pantheon_bridge)

add_executable(test_adversarial_bridge
    test_adversarial_bridge.cpp)
target_link_libraries(test_adversarial_bridge PRIVATE pantheon_layers)
add_test(NAME AdversarialBridgeTests COMMAND test_adversarial_bridge)

add_executable(test_adversarial_consensus
    test_adversarial_consensus.cpp)
target_link_libraries(test_adversarial_consensus PRIVATE pantheon_layers)
add_test(NAME AdversarialConsensusTests COMMAND test_adversarial_consensus)

add_executable(test_supply_invariants
    test_supply_invariants.cpp)
target_link_libraries(test_supply_invariants PRIVATE pantheon_layers)
add_test(NAME SupplyInvariantTests COMMAND test_supply_invariants)

add_executable(test_chain_id_separation
    test_chain_id_separation.cpp)
target_link_libraries(test_chain_id_separation PRIVATE pantheon_layers)
add_test(NAME ChainIdSeparationTests COMMAND test_chain_id_separation)

add_executable(test_chaos_composite
    test_chaos_composite.cpp)
target_link_libraries(test_chaos_composite PRIVATE pantheon_layers)
add_test(NAME ChaosCompositeTests COMMAND test_chaos_composite)
