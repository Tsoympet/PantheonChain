# Fuzzing tests

add_executable(test_dex_fuzzing test_dex_fuzzing.cpp)
target_link_libraries(test_dex_fuzzing
    parthenon_dex
    parthenon_primitives
    parthenon_crypto
)
add_test(NAME dex_fuzzing COMMAND test_dex_fuzzing)
