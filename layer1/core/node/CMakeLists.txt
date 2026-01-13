# Node module

add_library(parthenon_node STATIC
    node.cpp
)

target_include_directories(parthenon_node PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/layer1
)

target_link_libraries(parthenon_node PUBLIC
    parthenon_chainstate
    parthenon_validation
    parthenon_mempool
    parthenon_p2p
    parthenon_primitives
    parthenon_storage
)
