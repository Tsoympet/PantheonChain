add_executable(test_block_storage test_block_storage.cpp)
target_link_libraries(test_block_storage PRIVATE
    parthenon_storage
    parthenon_primitives
    parthenon_crypto
    parthenon_consensus
)
add_test(NAME test_block_storage COMMAND test_block_storage)
