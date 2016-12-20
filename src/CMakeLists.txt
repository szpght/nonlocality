set(COMMON_SOURCES nonlocality.c conn_vector.c)
set(ALL_SOURCES ${COMMON_SOURCES} server.c client.c)

add_custom_command(OUTPUT version.c
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_version.sh
        DEPENDS ${ALL_SOURCES})

set(SERVER_SOURCES server.c version.c ${COMMON_SOURCES})
add_executable(nonlocality-server ${SERVER_SOURCES})
target_link_libraries(nonlocality-server pthread)

set(CLIENT_SOURCES client.c version.c ${COMMON_SOURCES})
add_executable(nonlocality-client ${CLIENT_SOURCES})
target_link_libraries(nonlocality-client pthread)