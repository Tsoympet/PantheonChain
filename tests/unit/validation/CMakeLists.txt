# Validation tests

add_executable(test_validation test_validation.cpp)
target_link_libraries(test_validation PRIVATE
    parthenon_validation
    parthenon_chainstate
    parthenon_consensus
    parthenon_primitives
    parthenon_crypto
)
add_test(NAME test_validation COMMAND test_validation)
