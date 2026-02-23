# Governance unit tests

add_executable(test_governance
    test_governance.cpp
)

target_link_libraries(test_governance PRIVATE
    parthenon_governance
    parthenon_crypto
)

add_test(NAME test_governance COMMAND test_governance)
