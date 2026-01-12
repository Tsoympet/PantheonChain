# Mempool tests

add_executable(test_mempool test_mempool.cpp)
target_link_libraries(test_mempool PRIVATE
    parthenon_mempool
    parthenon_chainstate
    parthenon_validation
    parthenon_primitives
    parthenon_crypto
)
add_test(NAME test_mempool COMMAND test_mempool)
