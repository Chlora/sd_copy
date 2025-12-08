/**
 * @file zk_util.h
 * @brief Common ZooKeeper utilities for client and server
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#ifndef _ZK_UTIL_H
#define _ZK_UTIL_H

#include <zookeeper/zookeeper.h>
#include <stddef.h>

#define CHAIN_PATH "/chain"
#define TIMEOUT_MS 10000
#define CONNECT_TIMEOUT_SEC 5

/**
 * Wait for ZooKeeper connection to be established
 * 
 * @param zh ZooKeeper handle
 * @param timeout_sec Maximum seconds to wait
 * @return 0 on success, -1 on timeout or error
 */
int zk_wait_for_connection(zhandle_t *zh, int timeout_sec);

/**
 * Get sorted list of chain node paths
 * 
 * @param zh ZooKeeper handle
 * @param count Output parameter for number of nodes
 * @return Array of node paths (must be freed with zk_free_node_paths), or NULL on error
 */
char **zk_get_sorted_chain_nodes(zhandle_t *zh, int *count);

/**
 * Free array of node paths
 * 
 * @param paths Array of paths
 * @param count Number of paths
 */
void zk_free_node_paths(char **paths, int count);

/**
 * Get address (IP:port) stored in a ZooKeeper node
 * 
 * @param zh ZooKeeper handle
 * @param node_path Full path to node
 * @param buffer Output buffer for address
 * @param size Size of buffer
 * @return 0 on success, -1 on error
 */
int zk_get_node_address(zhandle_t *zh, const char *node_path, 
                        char *buffer, size_t size);

/**
 * Get address from a specific index in the sorted node array
 * 
 * @param zh ZooKeeper handle
 * @param node_paths Sorted array of node paths
 * @param index Index in array
 * @param buffer Output buffer for address
 * @param size Size of buffer
 * @return 0 on success, -1 on error
 */
int zk_get_node_address_at_index(zhandle_t *zh, char **node_paths, int index,
                                 char *buffer, size_t size);

/**
 * Find index of a specific node path in sorted array
 * 
 * @param node_paths Sorted array of node paths
 * @param count Number of paths
 * @param target_path Path to find
 * @return Index if found, -1 if not found
 */
int zk_find_node_index(char **node_paths, int count, const char *target_path);

#endif