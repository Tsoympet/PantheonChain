cmake_minimum_required(VERSION 3.15)

# parthenond executable
find_package(Threads REQUIRED)
add_executable(parthenond
    main.cpp
)

target_link_libraries(parthenond PRIVATE
    Threads::Threads
)

target_compile_features(parthenond PRIVATE cxx_std_17)

# Keep the default daemon configuration alongside build artifacts so
# platform-specific packaging scripts can bundle it from the build tree.
configure_file(parthenond.conf parthenond.conf COPYONLY)

install(TARGETS parthenond DESTINATION bin)
install(FILES parthenond.conf DESTINATION etc/parthenon)
