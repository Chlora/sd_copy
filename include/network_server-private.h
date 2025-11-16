/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 4/10/2025
 */

#ifndef _NETWORK_SERVER_PRIVATE_H
#define _NETWORK_SERVER_PRIVATE_H

#include "list.h"

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 5

struct client_params {
    int socket;
    struct sockaddr_in address;
    struct list_t *list;
};

void *serve_client(void *params);

#endif