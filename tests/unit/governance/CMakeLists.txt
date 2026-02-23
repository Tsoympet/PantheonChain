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

add_executable(test_treasury
    test_treasury.cpp
)
target_link_libraries(test_treasury PRIVATE
    parthenon_governance
)
add_test(NAME test_treasury COMMAND test_treasury)

add_executable(test_staking
    test_staking.cpp
)
target_link_libraries(test_staking PRIVATE
    parthenon_governance
)
add_test(NAME test_staking COMMAND test_staking)

add_executable(test_emergency
    test_emergency.cpp
)
target_link_libraries(test_emergency PRIVATE
    parthenon_governance
)
add_test(NAME test_emergency COMMAND test_emergency)

add_executable(test_eventlog
    test_eventlog.cpp
)
target_link_libraries(test_eventlog PRIVATE
    parthenon_governance
)
add_test(NAME test_eventlog COMMAND test_eventlog)

add_executable(test_fee_router
    test_fee_router.cpp
)
target_link_libraries(test_fee_router PRIVATE
    parthenon_governance
)
add_test(NAME test_fee_router COMMAND test_fee_router)

add_executable(test_supply_policy
    test_supply_policy.cpp
)
target_link_libraries(test_supply_policy PRIVATE
    parthenon_governance
)
add_test(NAME test_supply_policy COMMAND test_supply_policy)

add_executable(test_snapshot
    test_snapshot.cpp
)
target_link_libraries(test_snapshot PRIVATE
    parthenon_governance
)
add_test(NAME test_snapshot COMMAND test_snapshot)

add_executable(test_vesting
    test_vesting.cpp
)
target_link_libraries(test_vesting PRIVATE
    parthenon_governance
)
add_test(NAME test_vesting COMMAND test_vesting)

add_executable(test_veto_and_params
    test_veto_and_params.cpp
)
target_link_libraries(test_veto_and_params PRIVATE
    parthenon_governance
    parthenon_crypto
)
add_test(NAME test_veto_and_params COMMAND test_veto_and_params)
