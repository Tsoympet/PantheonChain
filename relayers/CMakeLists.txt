add_executable(pantheon-relayer-l2 pantheon-relayer-l2.cpp)
target_link_libraries(pantheon-relayer-l2 PRIVATE pantheon_layers)

add_executable(pantheon-relayer-l3 pantheon-relayer-l3.cpp)
target_link_libraries(pantheon-relayer-l3 PRIVATE pantheon_layers)
