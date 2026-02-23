cmake_minimum_required(VERSION 3.14)

add_library(parthenon_governance
    voting.cpp
    voting.h
    antiwhale.cpp
    antiwhale.h
    boule.cpp
    boule.h
    ostracism.cpp
    ostracism.h
    params.cpp
    params.h
    treasury.cpp
    treasury.h
    staking.cpp
    staking.h
    emergency.cpp
    emergency.h
    eventlog.cpp
    eventlog.h
    fee_router.cpp
    fee_router.h
    supply_policy.cpp
    supply_policy.h
    snapshot.cpp
    snapshot.h
    vesting.cpp
    vesting.h
)

target_include_directories(parthenon_governance PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(parthenon_governance PUBLIC
    parthenon_crypto
)
