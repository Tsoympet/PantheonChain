# Compatibility CMake entrypoint.
# Relayer implementation sources live under src/relayers in the layered architecture.

add_executable(pantheon-relayer-l2 ../src/relayers/pantheon-relayer-l2.cpp)
target_link_libraries(pantheon-relayer-l2 PRIVATE pantheon_layers)

add_executable(pantheon-relayer-l3 ../src/relayers/pantheon-relayer-l3.cpp)
target_link_libraries(pantheon-relayer-l3 PRIVATE pantheon_layers)
