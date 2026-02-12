cmake_minimum_required(VERSION 3.15)

# parthenond executable
find_package(Threads REQUIRED)
add_executable(parthenond
    main.cpp
)

target_link_libraries(parthenond PRIVATE
    Threads::Threads
    parthenon_node
    parthenon_wallet
    parthenon_rpc
)

target_compile_features(parthenond PRIVATE cxx_std_17)

# Keep the daemon configuration alongside build artifacts so
# platform-specific packaging scripts can bundle it from the build tree.
set(PARTHENOND_CONFIG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/parthenond.conf")
if(NOT EXISTS "${PARTHENOND_CONFIG_SOURCE}")
    set(PARTHENOND_CONFIG_SOURCE "${CMAKE_SOURCE_DIR}/parthenond.conf.example")
endif()

configure_file(
    "${PARTHENOND_CONFIG_SOURCE}"
    "${CMAKE_CURRENT_BINARY_DIR}/parthenond.conf"
    COPYONLY
)

install(TARGETS parthenond DESTINATION bin)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/parthenond.conf" DESTINATION etc/parthenon)
