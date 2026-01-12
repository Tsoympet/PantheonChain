# Primitives unit tests

add_executable(test_amount
    test_amount.cpp
)

target_link_libraries(test_amount PRIVATE
    parthenon_primitives
)

add_test(NAME test_amount COMMAND test_amount)

add_executable(test_asset
    test_asset.cpp
)

target_link_libraries(test_asset PRIVATE
    parthenon_primitives
)

add_test(NAME test_asset COMMAND test_asset)
