#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

int server_init(const char *host, const uint16_t port);
void server_run();
void server_cleanup();

#endif // SERVER_H
