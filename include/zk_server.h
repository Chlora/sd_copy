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


/**
 * Establishes connection to ZooKeeper ensemble and initializes node address.
 * @param zk_host ZooKeeper connection string (e.g., "localhost:2181")
 * @param node_addr Address of this server node (e.g., "localhost:8080")
 * @return 0 on success, -1 on failure
 */
int zk_connect(const char *zk_host, const char *node_addr);

/**
 * Registers this server node in ZooKeeper by creating an ephemeral sequential znode.
 * The node will be added to the chain replication group.
 * @return 0 on success, -1 on failure
 */
int zk_register();

/**
 * Updates the local view of the chain replication topology by reading ZooKeeper.
 * Determines this node's role (single, head, middle, tail) based on chain position.
 * @return 0 on success, -1 on failure
 */
int zk_update_chain();

/**
 * Closes the connection to ZooKeeper and cleans up resources.
 */
void zk_disconnect();


/**
 * Forwards a message to the next server in the chain replication topology.
 * Used by head and middle nodes to propagate operations down the chain.
 * @param msg The message to forward to the next node
 * @return 0 on success, -1 on failure
 */
int zk_forward(MessageT *msg);

/**
 * Synchronizes a remote list with the previous node in the chain.
 * @param list The list to be synchronize
 * @return 0 on success, -1 on failure
 */
int zk_sync(struct list_t *list);


/**
 * Prints the current ZooKeeper connection status and chain topology information.
 * Displays role, chain position, size, and epoch to standard output.
 */
void zk_print_status();

/**
 * Converts a role enumeration value to its string representation.
 * @param role The role to convert (SINGLE, HEAD, MIDDLE, or TAIL)
 * @return String representation of the role
 */
const char *zk_role_string(zk_role_t role);

/**
 * Gets this node's position in the chain replication topology.
 * @return position index in the chain
 */
int zk_get_chain_position();

/**
 * Gets the total number of nodes in the chain.
 * @return Number of servers in the current chain
 */
int zk_get_chain_size();

#endif