add_executable(pantheon-node pantheon_node.cpp)
target_link_libraries(pantheon-node PRIVATE pantheon_layers)

add_executable(pantheon-cli pantheon_cli.cpp)
target_link_libraries(pantheon-cli PRIVATE pantheon_layers)
