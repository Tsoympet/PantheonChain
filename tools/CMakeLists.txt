# Tools and SDK libraries

find_package(OpenSSL REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG OFF)
find_package(Threads REQUIRED)

add_library(parthenon_mobile_sdk STATIC
    mobile_sdks/mobile_sdk.cpp
)

target_include_directories(parthenon_mobile_sdk PUBLIC
    ${CMAKE_SOURCE_DIR}/tools/mobile_sdks
    ${CMAKE_SOURCE_DIR}/layer1/core
    ${CMAKE_SOURCE_DIR}/third_party/stubs/json/include
)

target_link_libraries(parthenon_mobile_sdk PUBLIC
    parthenon_crypto
    OpenSSL::Crypto
    Threads::Threads
)

if(WIN32)
    target_link_libraries(parthenon_mobile_sdk PUBLIC ws2_32)
endif()
