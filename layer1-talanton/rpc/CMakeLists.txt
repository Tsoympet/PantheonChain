# RPC module

set(THREADS_PREFER_PTHREAD_FLAG OFF)
find_package(Threads REQUIRED)
add_library(parthenon_rpc STATIC
    rpc_server.cpp
)

target_include_directories(parthenon_rpc PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1-talanton/core
    ${CMAKE_SOURCE_DIR}/layer1-talanton
    ${CMAKE_SOURCE_DIR}/third_party/stubs/cpp-httplib
)

target_link_libraries(parthenon_rpc PUBLIC
    parthenon_monetary
    parthenon_chainstate
    parthenon_primitives
    parthenon_node
    parthenon_wallet
    parthenon_governance
    leveldb
    Threads::Threads
)
