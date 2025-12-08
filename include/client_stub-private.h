/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 4/10/2025
 */
#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "client_stub.h"

struct rlist_t
{
    char *server_address;
    int server_port;
    int sockfd;
};

/**
 * Obtém todos os carros da lista remota.
 */
struct data_t **rlist_get_all(struct rlist_t *rlist);

#endif