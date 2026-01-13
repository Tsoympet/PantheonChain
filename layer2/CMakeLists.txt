cmake_minimum_required(VERSION 3.10)

add_library(layer2
    payment_channels/payment_channel.cpp
    bridges/htlc/htlc.cpp
    bridges/spv/spv_bridge.cpp
)

target_include_directories(layer2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../layer1/core
)

target_link_libraries(layer2
    parthenon_crypto
    parthenon_primitives
)

# Note: Legacy paths (channels/, htlc/, spv/) maintained for backward compatibility
# New code should use payment_channels/, bridges/htlc/, bridges/spv/
