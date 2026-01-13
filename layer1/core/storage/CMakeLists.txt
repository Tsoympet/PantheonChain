# Storage module

add_library(parthenon_storage STATIC
    block_storage.cpp
    utxo_storage.cpp
)

target_include_directories(parthenon_storage PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/layer1
)

target_link_libraries(parthenon_storage PUBLIC
    parthenon_primitives
    parthenon_chainstate
    leveldb
)
