#pragma once

typedef struct {
    uint16_t control_port;
    uint16_t tunneled_port;
    uint16_t data_port;
} ServerConfig;

void server(ServerConfig config);
ServerConfig load_config();