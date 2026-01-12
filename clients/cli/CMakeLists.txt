cmake_minimum_required(VERSION 3.15)

# parthenon-cli executable
add_executable(parthenon-cli
    main.cpp
)

target_compile_features(parthenon-cli PRIVATE cxx_std_17)

install(TARGETS parthenon-cli DESTINATION bin)
