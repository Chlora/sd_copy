/**
 * @file zookeeper-interactions.h
 * 
 * @brief Client-side interface for interacting with Zookeper
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#ifndef _ZOOKEPER_INTERACTIONS_H
#define _ZOOKEPER_INTERACTIONS_H

#include "client_stub-private.h"

struct rlist_t;

extern struct rlist_t *rlistHead;
extern struct rlist_t *rlistTail;

int connectToZookeper(char* ip);

int areConnectionsEstablished();

int disconnectFromZookeeper();


#endif