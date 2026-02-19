add_executable(test_layers test_layers.cpp)
target_link_libraries(test_layers PRIVATE pantheon_layers)
add_test(NAME LayersTests COMMAND test_layers)
