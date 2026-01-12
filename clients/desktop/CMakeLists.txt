cmake_minimum_required(VERSION 3.15)

# Desktop wallet - requires Qt for production
# This is a scaffold version

add_executable(parthenon-qt
    gui/main.cpp
)

target_compile_features(parthenon-qt PRIVATE cxx_std_17)

# Production version will use:
# find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
# target_link_libraries(parthenon-qt Qt5::Widgets Qt5::Network)

install(TARGETS parthenon-qt DESTINATION bin)
