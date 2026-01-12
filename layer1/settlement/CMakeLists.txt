cmake_minimum_required(VERSION 3.14)

# Settlement module library
add_library(settlement
    destination_tag.cpp
    destination_tag.h
    escrow.cpp
    escrow.h
    multisig.cpp
    multisig.h
)

target_include_directories(settlement PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(settlement PUBLIC
    parthenon_crypto
    parthenon_primitives
)
