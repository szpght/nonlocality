set(SERVER_SOURCES server.c nonlocality.c conn_vector.c)
add_executable(nonlocality-server ${SERVER_SOURCES})
target_link_libraries(nonlocality-server pthread)

set(CLIENT_SOURCES client.c nonlocality.c)
add_executable(nonlocality-client ${CLIENT_SOURCES})
