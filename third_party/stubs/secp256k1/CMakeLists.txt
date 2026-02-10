add_library(secp256k1 STATIC secp256k1_stub.c)

target_include_directories(secp256k1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
