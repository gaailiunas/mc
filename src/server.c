#include "server.h"
#include <stdio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct tcp_client {
    int fd;
    /* ... */
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

static void server_handle(int fd, char *data, int nread)
{
    printf("read %d bytes from %d fd\n", nread, fd);
}

void server_run()
{
    struct pollfd fds[MAX_CLIENTS];
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
                    nfds++;
                }
                else {
                    char buf[4096];
                    int nread = read(fds[i].fd, buf, sizeof(buf));
                    if (nread <= 0) {
                        printf("encountered a disconnection/error from %d fd\n",
                               fds[i].fd);
                        close(fds[i].fd);

                        /* temporary solution */
                        for (int j = i; j < nfds - 1; j++)
                            fds[j] = fds[j + 1];
                        nfds--;
                        i--;
                        continue;
                    }

                    server_handle(fds[i].fd, buf, nread);
                }
            }
        }
    }
}

void server_cleanup() { close(server_fd); }
