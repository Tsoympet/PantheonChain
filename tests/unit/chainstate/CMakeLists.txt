# ChainState tests

add_executable(test_chainstate test_chainstate.cpp)
target_link_libraries(test_chainstate PRIVATE
    parthenon_chainstate
    parthenon_consensus
    parthenon_primitives
    parthenon_crypto
)
add_test(NAME test_chainstate COMMAND test_chainstate)

add_executable(test_utxo test_utxo.cpp)
target_link_libraries(test_utxo PRIVATE
    parthenon_chainstate
    parthenon_primitives
)
add_test(NAME test_utxo COMMAND test_utxo)

add_executable(test_chain test_chain.cpp)
target_link_libraries(test_chain PRIVATE
    parthenon_chainstate
    parthenon_consensus
    parthenon_primitives
    parthenon_crypto
)
add_test(NAME test_chain COMMAND test_chain)
