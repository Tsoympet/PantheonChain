# Layer2 tests
add_executable(test_layer2 test_layer2.cpp)

target_link_libraries(test_layer2 PRIVATE
    layer2
    parthenon_crypto
    parthenon_primitives
)

add_test(NAME test_layer2 COMMAND test_layer2)
