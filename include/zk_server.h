#ifndef _ZK_SERVER_H
#define _ZK_SERVER_H

#include <zookeeper/zookeeper.h>
#include <stdint.h>
#include "sdmessage.pb-c.h"

struct list_t;

typedef enum {
    ZK_ROLE_SINGLE,
    ZK_ROLE_HEAD,
    ZK_ROLE_MIDDLE,
    ZK_ROLE_TAIL
} zk_role_t;

// Initialization
int zk_connect(const char *zk_host, const char *node_addr);
int zk_register(void);
int zk_update_chain(void);
void zk_disconnect(void);

// Chain replication
int zk_forward(MessageT *msg);
int zk_sync(struct list_t *list);

// Info getters
void zk_print_status(void);
const char *zk_role_string(zk_role_t role);
int zk_get_chain_position(void);
int zk_get_chain_size(void);
uint64_t zk_get_chain_epoch(void);

#endif