# Identity library (DID)
add_library(parthenon_identity STATIC
    did.cpp
)

target_include_directories(parthenon_identity PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

target_link_libraries(parthenon_identity PUBLIC
    parthenon_crypto
)
