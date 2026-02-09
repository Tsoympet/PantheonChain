# RPC module

find_package(Threads REQUIRED)
add_library(parthenon_rpc STATIC
    rpc_server.cpp
)

target_include_directories(parthenon_rpc PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/layer1
)

target_link_libraries(parthenon_rpc PUBLIC
    parthenon_chainstate
    parthenon_primitives
    parthenon_node
    parthenon_wallet
    leveldb
    Threads::Threads
)
