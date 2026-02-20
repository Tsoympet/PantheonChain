cmake_minimum_required(VERSION 3.15)

# parthenon-cli executable
add_executable(parthenon-cli
    main.cpp
)

target_compile_features(parthenon-cli PRIVATE cxx_std_17)

install(TARGETS parthenon-cli DESTINATION bin)


target_include_directories(parthenon-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/layer1/core
)

target_link_libraries(parthenon-cli PRIVATE
    parthenon_monetary
)
