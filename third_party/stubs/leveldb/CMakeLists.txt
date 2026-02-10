add_library(leveldb STATIC leveldb_stub.cc)

target_include_directories(leveldb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
