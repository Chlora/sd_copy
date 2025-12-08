/**
 * @file nclient_stub.c
 * 
 * @brief Client stub implementation for remote list operations
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include "data.h"
#include "list.h"
#include "client_stub-private.h"
#include "network_client.h"
#include "message-private.h"


#include "arpa/inet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    
    Data *proto_car = data_to_pb(car);
    if(proto_car == NULL) return -1;
    msg.data = proto_car;

    // Send to server
    MessageT *reply = network_send_receive(rlist, &msg);

    free(proto_car->modelo);
    free(proto_car); 
    if (reply == NULL) {
        return -1;
    }

    int result = -1;
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
int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo){
    if(rlist==NULL || modelo==NULL) return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode=MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MODEL;
    
    msg.models = malloc(sizeof(char *) * 1);
    if(msg.models == NULL) return -1;
    
    // Duplicate the string to avoid issues with memory ownership
    msg.models[0] = strdup(modelo);
    if(msg.models[0] == NULL) {
        free(msg.models);
        return -1;
    }
    msg.n_models = 1;

    MessageT *reply = network_send_receive(rlist, &msg);

    // Free the allocated memory
    free(msg.models[0]);
    free(msg.models);

    if(reply == NULL) return -1;

    int result = -1;
    if(reply->opcode ==MESSAGE_T__OPCODE__OP_DEL + 1) 
        result = reply->result;

    message_t__free_unpacked(reply, NULL);
    return result;
    
}

/* Obtém o primeiro carro que corresponda à marca indicada.
 * Retorna ponteiro para os dados ou NULL se não encontrar ou em caso de erro.
 */
struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca){
    if(rlist==NULL) return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode=MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MARCA;
    
    Data *tmp = malloc(sizeof(Data));
    if(tmp==NULL) return NULL;
    data__init(tmp);
    tmp->marca = (Marca) marca;

    msg.data=tmp;

    MessageT *reply = network_send_receive(rlist, &msg);

    free(tmp);

    if(reply == NULL) return NULL;

    struct data_t *result = NULL;
    if(reply->opcode ==MESSAGE_T__OPCODE__OP_GET + 1 && reply->data) 
        result = pb_to_data(reply->data);

    message_t__free_unpacked(reply, NULL);
    return result;
}

/* Obtém um array de ponteiros para carros de um determinado ano.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano){

    if(rlist==NULL) return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode= MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_YEAR;
    
    Data *tmp = malloc(sizeof(Data));
    if(tmp==NULL) return NULL;
    data__init(tmp);
    tmp->ano = ano;

    msg.data=tmp;

    MessageT *reply = network_send_receive(rlist, &msg);

    free(tmp);

    if(reply == NULL) return NULL;

    struct data_t **array = NULL;
    if(reply->opcode == MESSAGE_T__OPCODE__OP_GET + 1 && reply->n_cars >0) {
        array = malloc((reply->n_cars + 1) * sizeof(struct data_t *));
        if(array){
            for(size_t i = 0; i< reply->n_cars; i++)
                array[i] = pb_to_data(reply->cars[i]);
            array[reply->n_cars] = NULL;
        }
    }

    message_t__free_unpacked(reply, NULL);
    return array;
}

/* Ordena a lista remota de carros por ano de fabrico (crescente).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_order_by_year(struct rlist_t *rlist){
    if(rlist==NULL) return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode=MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *reply = network_send_receive(rlist, &msg);
    if(reply==NULL) return -1;

    int res = (reply->opcode == MESSAGE_T__OPCODE__OP_GETLISTBYTEAR +1 ) ? 0: -1;

    struct data_t **array = pb_to_data_array(reply->cars, reply->n_cars);

    printf("Lista de carros ordenada por ano:\n");
    for (int i = 0; array[i] != NULL; i++) {
        struct data_t *car = array[i];
        printf("Modelo: %s, Marca: %d, Ano: %d\n", car->modelo, car->marca, car->ano);
        data_destroy(car);
    }
    free(array);

    message_t__free_unpacked(reply,NULL);
    return res;
}

/* Retorna o número de carros na lista remota ou -1 em caso de erro.
 */
int rlist_size(struct rlist_t *rlist){
    if(rlist==NULL) return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode=MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *reply = network_send_receive(rlist, &msg);
    if(reply==NULL) return -1;

    int size = -1;
    if(reply->opcode == MESSAGE_T__OPCODE__OP_SIZE +1 ) size = reply->result;
    message_t__free_unpacked(reply,NULL);
    return size;
}

/* Constrói um array de strings com os modelos dos carros na lista remota.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
char **rlist_get_model_list(struct rlist_t *rlist){
    if(rlist==NULL) return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode=MESSAGE_T__OPCODE__OP_GETMODELS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *reply = network_send_receive(rlist, &msg);
    if(reply==NULL) return NULL;

    char **models = NULL;
    if (reply->opcode == MESSAGE_T__OPCODE__OP_GETMODELS +1 && reply->n_models > 0 ){
        models = malloc((reply->n_models +1) * sizeof(char *));
        if(models){
            for(size_t i = 0; i < reply->n_models; i++)
                models[i] = strdup(reply->models[i]);
            models[reply->n_models] = NULL;
        }
    }

    message_t__free_unpacked(reply,NULL);
    return models;
}

/* Liberta a memória ocupada pelo array de modelos.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_free_model_list(char **models){
    if (models == NULL) return -1;

    for (int i =0; models[i] != NULL; i++){
        free(models[i]);
    }
    free(models);
    return 0;
}

struct data_t **rlist_get_all(struct rlist_t *rlist){
    if(rlist==NULL) return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode=MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *reply = network_send_receive(rlist, &msg);
    if(reply==NULL) return -1;

    struct data_t **array = (reply->opcode == MESSAGE_T__OPCODE__OP_GETLISTBYTEAR +1 ) ? pb_to_data_array(reply->cars, reply->n_cars): NULL;

    message_t__free_unpacked(reply,NULL);
    return array;
}