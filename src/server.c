#include "server.h"
#include "var.h"
#include <stdio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>

typedef struct tcp_client {
    int fd;
    int state;
} tcp_client_t;

static int server_fd = -1;

#define MAX_CLIENTS 1024

/* seconds */
#define POLL_TIMEOUT 1

int server_init(const char *host, const uint16_t port)
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        fprintf(stderr, "Failed to open a socket\n");
        return 1;
    }

    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &sin.sin_addr) != 1) {
        fprintf(stderr,
                "Failed to convert numerical IP address to network order\n");
        close(server_fd);
        return 1;
    }

    if (bind(server_fd, (const struct sockaddr *)&sin, sizeof(sin)) != 0) {
        fprintf(stderr, "Failed to bind the socket\n");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) != 0) {
        fprintf(stderr, "Failed to listen to the socket\n");
        close(server_fd);
        return 1;
    }

    return 0;
}

static tcp_client_t *client_new(int fd)
{
    tcp_client_t *c = (tcp_client_t *)malloc(sizeof(tcp_client_t));
    if (c) {
        c->fd = fd;
        c->state = 0;
    }
    return c;
}

void client_free(tcp_client_t *client) { free(client); }

static void handshake(tcp_client_t *client, char *data, int nread)
{
    int offset = 0;

    varint_t packet_length = read_varint(data, nread);
    offset += packet_length.byteslen;

    varint_t packet_id = read_varint(data + offset, nread - offset);
    printf("packet_id: %d\n", packet_id.value);
    offset += packet_id.byteslen;

    varint_t protocol_version = read_varint(data + offset, nread - offset);
    offset += protocol_version.byteslen;

    varint_t string_length = read_varint(data + offset, nread - offset);
    offset += string_length.byteslen;

    char server_address[256] = {0};
    memcpy(server_address, data + offset, string_length.value);
    offset += string_length.value;

    unsigned short port =
        (unsigned char)data[offset] << 8 | (unsigned char)data[offset + 1];
    offset += 2;

    varint_t next_state = read_varint(data + offset, nread - offset);
    client->state = next_state.value;

    printf("Handshake packet:\n");
    printf("Protocol Version: %d\n", protocol_version.value);
    printf("Server Address: %s\n", server_address);
    printf("Server Port: %d\n", port);
    printf("Next State: %d\n", next_state.value);
}

static void server_handle(tcp_client_t *client, char *data, int nread)
{
    printf("read %d bytes from %d fd\n", nread, client->fd);

    printf("--- data dump ---\n");
    printf("* chars: ");
    for (int i = 0; i < nread; i++)
        printf("%c ", data[i]);
    printf("\n");
    printf("* bytes: ");
    for (int i = 0; i < nread; i++)
        printf("%d ", data[i]);
    printf("\n");
    printf("* hex: ");
    for (int i = 0; i < nread; i++)
        printf("%02x ", data[i]);
    printf("\n");
    printf("-----------------\n");

    switch (client->state) {
    case 0: {
        printf("*** handshake\n");
        handshake(client, data, nread);
        break;
    }
    case 1: {
        printf("*** status\n");
        break;
    }
    }
}

void server_run()
{
    struct pollfd fds[MAX_CLIENTS];
    tcp_client_t *clients[MAX_CLIENTS];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    int nfds = 1;

    while (1) {
        int ret = poll(fds, nfds, POLL_TIMEOUT);
        if (ret == -1) {
            fprintf(stdout, "poll error\n");
            break;
        }
        if (ret == 0)
            continue;

        for (int i = 0; i < nfds; i++) {
            /* if readable */
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd && nfds - 1 < MAX_CLIENTS) {
                    int client_fd = accept(server_fd, NULL, NULL);
                    if (client_fd == -1) {
                        fprintf(stderr, "Invalid client fd\n");
                        continue;
                    }

                    printf("accepted client fd: %d\n", client_fd);

                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN;
                    clients[nfds] = client_new(client_fd);
                    nfds++;
                }
                else {
                    char buf[4096];
                    int nread = read(fds[i].fd, buf, sizeof(buf));
                    if (nread <= 0) {
                        printf("encountered a disconnection/error from %d fd\n",
                               fds[i].fd);
                        close(fds[i].fd);

                        client_free(clients[i]);
                        clients[i] = NULL;

                        /* temporary solution */
                        for (int j = i; j < nfds - 1; j++)
                            fds[j] = fds[j + 1];
                        nfds--;
                        i--;
                        continue;
                    }

                    server_handle(clients[i], buf, nread);
                }
            }
        }
    }
}

void server_cleanup() { close(server_fd); }
