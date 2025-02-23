#include "server.h"
#include <assert.h>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 6969

int main(void)
{
    assert(server_init(SERVER_HOST, SERVER_PORT) == 0);
    server_run(); 
    server_cleanup();
    return 0;
}
