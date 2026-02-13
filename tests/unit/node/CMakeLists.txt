# Node parameter tests

add_executable(test_chainparams test_chainparams.cpp)
target_link_libraries(test_chainparams PRIVATE
    parthenon_node
)
add_test(NAME test_chainparams COMMAND test_chainparams)
