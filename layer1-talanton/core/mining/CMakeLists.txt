# Mining module

add_library(parthenon_mining STATIC
    miner.cpp
)

target_include_directories(parthenon_mining PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/layer1
)

target_link_libraries(parthenon_mining PUBLIC
    parthenon_crypto
    parthenon_primitives
    parthenon_consensus
    parthenon_chainstate
)
