add_executable(test_monetary_units test_monetary_units.cpp)
target_link_libraries(test_monetary_units PRIVATE
    parthenon_monetary
    parthenon_rpc
)
add_test(NAME test_monetary_units COMMAND test_monetary_units)
