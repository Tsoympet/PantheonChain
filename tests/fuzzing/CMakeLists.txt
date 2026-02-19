# Fuzzing tests

add_executable(test_dex_fuzzing test_dex_fuzzing.cpp)

# Add include directories for layer2 headers
target_include_directories(test_dex_fuzzing PRIVATE
    ${CMAKE_SOURCE_DIR}/layer2
    ${CMAKE_SOURCE_DIR}/layer1/core
)

target_link_libraries(test_dex_fuzzing
    layer2
    parthenon_primitives
    parthenon_crypto
)
add_test(NAME dex_fuzzing COMMAND test_dex_fuzzing)

add_executable(test_layers_fuzzing test_layers_fuzzing.cpp)
target_link_libraries(test_layers_fuzzing
    pantheon_layers
)
add_test(NAME layers_fuzzing COMMAND test_layers_fuzzing)
