cmake_minimum_required(VERSION 3.10)

add_executable(test_layer2 test_layer2.cpp)

target_link_libraries(test_layer2
    layer2
    crypto
    primitives
)

add_test(NAME test_layer2 COMMAND test_layer2)
