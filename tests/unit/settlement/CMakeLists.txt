# Settlement tests
add_executable(test_settlement test_settlement.cpp)

target_link_libraries(test_settlement PRIVATE
    settlement
    parthenon_crypto
    parthenon_primitives
)

add_test(NAME test_settlement COMMAND test_settlement)
