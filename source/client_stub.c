#include "../include/data.h"
#include "../include/list.h"

#include "../include/client_stub-private.h"
#include "../include/network_client.h"

#include "arpa/inet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int eventDC = 0;

/* Remote list, deve conter as informações necessárias para comunicar
 * com o servidor. A definir pelo grupo em client_stub-private.h
 */

struct rlist_t; /* Definida em list-private.h */

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna a estrutura rlist preenchida, ou NULL em caso de erro.
 */
struct rlist_t *rlist_connect(char *address_port) {

    if (!address_port) return NULL;

    const char *colon_ptr = strchr(address_port, ':');
    if (!colon_ptr) return NULL;

    size_t host_len = (size_t)(colon_ptr - address_port);
    const char *port_str = colon_ptr + 1;

    char *hostname = malloc(host_len + 1);
    if (!hostname) return NULL;
    memcpy(hostname, address_port, host_len);
    hostname[host_len] = '\0';

    int port = atoi(port_str);
    if (port <= 0 || port > 65535) {
        free(hostname);
        return NULL;
    }

    struct rlist_t *rlist = malloc(sizeof *rlist);
    if (!rlist) {
        free(hostname);
        return NULL;
    }

    rlist->server_address = hostname;
    rlist->server_port = port;
    rlist->sockfd = -1;

    if (network_connect(rlist) != 0) {
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }

    return rlist;
}

/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem, ou -1 em caso de erro.
 */
int rlist_disconnect(struct rlist_t *rlist)
{
    if (rlist == NULL) {
        return -1;
    }
    if (network_close(rlist) != 0) {
        return -1;
    }

    free(rlist->server_address);
    free(rlist);

    return 0;
}

/* Adiciona um novo carro à lista remota.
 * O carro é inserido na última posição da lista.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */

int rlist_add(struct rlist_t *rlist, struct data_t *car) {
    if (rlist == NULL || car == NULL) {
        return -1;
    }

    // Initialize and populate message
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
    msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;
    msg.data = car;

    // Send to server
    MessageT *reply = network_send_receive(rlist, &msg);
    if (reply == NULL) {
        return -1;
    }

    int result;
    if (reply->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        result = -1;
    } else if (reply->opcode == MESSAGE_T__OPCODE__OP_ADD + 1) {
        result = 0;
    }

    message_t__free_unpacked(reply, NULL);
    return result;
}


/* Remove da lista remota o primeiro carro que corresponda ao modelo indicado.
 * Retorna 0 se encontrou e removeu, 1 se não encontrou, ou -1 em caso de erro.
 */
int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo);

/* Obtém o primeiro carro que corresponda à marca indicada.
 * Retorna ponteiro para os dados ou NULL se não encontrar ou em caso de erro.
 */
struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca);

/* Obtém um array de ponteiros para carros de um determinado ano.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano);

/* Ordena a lista remota de carros por ano de fabrico (crescente).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_order_by_year(struct rlist_t *rlist);

/* Retorna o número de carros na lista remota ou -1 em caso de erro.
 */
int rlist_size(struct rlist_t *rlist);

/* Constrói um array de strings com os modelos dos carros na lista remota.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
char **rlist_get_model_list(struct rlist_t *rlist);

/* Liberta a memória ocupada pelo array de modelos.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_free_model_list(char **models);