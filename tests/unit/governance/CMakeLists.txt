# Governance unit tests

add_executable(test_governance
    test_governance.cpp
)
target_link_libraries(test_governance PRIVATE
    parthenon_governance
    parthenon_crypto
)
add_test(NAME test_governance COMMAND test_governance)

add_executable(test_antiwhale
    test_antiwhale.cpp
)
target_link_libraries(test_antiwhale PRIVATE
    parthenon_governance
)
add_test(NAME test_antiwhale COMMAND test_antiwhale)

add_executable(test_boule
    test_boule.cpp
)
target_link_libraries(test_boule PRIVATE
    parthenon_governance
)
add_test(NAME test_boule COMMAND test_boule)

add_executable(test_ostracism
    test_ostracism.cpp
)
target_link_libraries(test_ostracism PRIVATE
    parthenon_governance
)
add_test(NAME test_ostracism COMMAND test_ostracism)

add_executable(test_params
    test_params.cpp
)
target_link_libraries(test_params PRIVATE
    parthenon_governance
)
add_test(NAME test_params COMMAND test_params)

add_executable(test_voting_integration
    test_voting_integration.cpp
)
target_link_libraries(test_voting_integration PRIVATE
    parthenon_governance
    parthenon_crypto
)
add_test(NAME test_voting_integration COMMAND test_voting_integration)
