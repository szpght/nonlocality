set(COMMON_SOURCES nonlocality.c conn_vector.c)

set(SERVER_SOURCES server.c ${COMMON_SOURCES})
add_executable(nonlocality-server ${SERVER_SOURCES})
target_link_libraries(nonlocality-server pthread)

set(CLIENT_SOURCES client.c ${COMMON_SOURCES})
add_executable(nonlocality-client ${CLIENT_SOURCES})
target_link_libraries(nonlocality-client pthread)