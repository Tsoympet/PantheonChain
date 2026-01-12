# Crypto unit tests

add_executable(test_sha256
    test_sha256.cpp
)

target_link_libraries(test_sha256 PRIVATE
    parthenon_crypto
)

add_test(NAME test_sha256 COMMAND test_sha256)

add_executable(test_schnorr
    test_schnorr.cpp
)

target_link_libraries(test_schnorr PRIVATE
    parthenon_crypto
)

add_test(NAME test_schnorr COMMAND test_schnorr)
