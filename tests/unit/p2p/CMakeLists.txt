# P2P tests

add_executable(test_p2p test_p2p.cpp)
target_link_libraries(test_p2p PRIVATE
    parthenon_p2p
    parthenon_primitives
    parthenon_crypto
)
add_test(NAME test_p2p COMMAND test_p2p)
