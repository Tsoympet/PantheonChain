cmake_minimum_required(VERSION 3.14)

add_library(parthenon_governance
    voting.cpp
    voting.h
)

target_include_directories(parthenon_governance PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(parthenon_governance PUBLIC
    parthenon_crypto
)
