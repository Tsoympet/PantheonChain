# P2P networking module

add_library(parthenon_p2p STATIC
    protocol.cpp
    message.cpp
    network_manager.cpp
)

target_include_directories(parthenon_p2p PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/layer1
)

target_link_libraries(parthenon_p2p PUBLIC
    parthenon_primitives
    parthenon_crypto
    pthread
)
