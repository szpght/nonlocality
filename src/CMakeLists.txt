set(CLIENT_SOURCES client.c nonlocality.c)
set(SERVER_SOURCES server.c nonlocality.c)
add_executable(nonlocality-client ${CLIENT_SOURCES})
add_executable(nonlocality-server ${SERVER_SOURCES})