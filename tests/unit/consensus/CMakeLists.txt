# Consensus tests

add_executable(test_difficulty test_difficulty.cpp)
target_link_libraries(test_difficulty PRIVATE
    parthenon_consensus
    parthenon_crypto
)
add_test(NAME test_difficulty COMMAND test_difficulty)

add_executable(test_issuance test_issuance.cpp)
target_link_libraries(test_issuance PRIVATE
    parthenon_consensus
    parthenon_crypto
)
add_test(NAME test_issuance COMMAND test_issuance)
