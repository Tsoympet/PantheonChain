# P2P networking module

# Find OpenSSL for TLS support
find_package(OpenSSL REQUIRED)

add_library(parthenon_p2p STATIC
    protocol.cpp
    message.cpp
    network_manager.cpp
    peer_database.cpp
    tls_connection.cpp
    zero_copy_network.cpp
    certificate_rotation.cpp
)

target_include_directories(parthenon_p2p PUBLIC
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/layer1
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(parthenon_p2p PUBLIC
    parthenon_primitives
    parthenon_crypto
    pthread
    OpenSSL::SSL
    OpenSSL::Crypto
)
