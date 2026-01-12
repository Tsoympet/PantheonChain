cmake_minimum_required(VERSION 3.14)

# Settlement tests
add_executable(test_settlement test_settlement.cpp)

target_link_libraries(test_settlement
    settlement
    crypto
)

add_test(NAME test_settlement COMMAND test_settlement)
