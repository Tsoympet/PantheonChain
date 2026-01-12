# ChainState tests

add_executable(test_chainstate test_chainstate.cpp)
target_link_libraries(test_chainstate PRIVATE
    parthenon_chainstate
    parthenon_consensus
    parthenon_primitives
    parthenon_crypto
)
add_test(NAME test_chainstate COMMAND test_chainstate)
