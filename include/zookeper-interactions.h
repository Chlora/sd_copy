/**
 * @file zookeeper-interactions.h
 * 
 * @brief Client-side interface for interacting with Zookeper
 * 
 * SD-12
 */

#ifndef _ZOOKEPER_INTERACTIONS_H
#define _ZOOKEPER_INTERACTIONS_H

#include "client_stub-private.h"

struct rlist_t;

extern struct rlist_t *rlistHead;
extern struct rlist_t *rlistTail;

/**
 * Establece ligação com o Zookeeper, preenchendo as globais rlistHead e rlistTail, e tanta establecer ligações aos servidores correspondentes.
 * Retorna 0 em caso de sucesso, ou -1 em caso de erro.
 */
int connectToZookeper(char* ip);

/**
 * Retorna 1 se todas as ligações estiverem establecidas, senão 0.
 */
int areConnectionsEstablished();

/**
 * Termina a ligação com o Zookeeper, assim também como aos servidores correspondentes às globais rlistHead e rlistTail.
 * Liberta toda a memória associada a estes.
 * Retorna 0 em caso de sucesso, ou -1 em caso de erro.
 */
int disconnectFromZookeeper();


#endif