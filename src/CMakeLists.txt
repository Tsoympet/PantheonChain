add_library(pantheon_layers STATIC
    common/commitments.cpp
    common/cryptography.cpp
    common/bridging.cpp
    common/mempool.cpp
    common/metrics.cpp
    common/p2p_network.cpp
    common/serialization.cpp
    common/storage.cpp
    talanton/l1_commitment_validator.cpp
    drachma/pos_consensus.cpp
    drachma/payments_state_machine.cpp
    obolos/pos_consensus.cpp
    obolos/execution.cpp
)

add_library(parthenon_monetary STATIC
    common/monetary/denominations.cpp
    common/monetary/units.cpp
)

target_include_directories(parthenon_monetary PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/layer1/core
)

target_include_directories(pantheon_layers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(pantheon_layers PRIVATE /W4 /WX)
else()
  target_compile_options(pantheon_layers PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_subdirectory(tools)


target_link_libraries(pantheon_layers PUBLIC parthenon_monetary)
