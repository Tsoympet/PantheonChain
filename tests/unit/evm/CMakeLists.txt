# EVM tests

add_executable(test_evm test_evm.cpp)
target_link_libraries(test_evm PRIVATE
    parthenon_evm
    parthenon_crypto
    parthenon_primitives
)
add_test(NAME test_evm COMMAND test_evm)
