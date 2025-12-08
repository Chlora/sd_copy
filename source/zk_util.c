/**
 * @file zk_util.c
 * @brief Common ZooKeeper utilities implementation
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#define THREADED

#include "zk_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* Internal utility for sorting paths */
static int pathcmp(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int zk_wait_for_connection(zhandle_t *zh, int timeout_sec) {
    if (!zh) return -1;
    
    time_t start = time(NULL);
    
    while (time(NULL) - start < timeout_sec) {
        int state = zoo_state(zh);
        
        if (state == ZOO_CONNECTED_STATE) {
            return 0;
        }
        
        if (state == ZOO_AUTH_FAILED_STATE || state == ZOO_EXPIRED_SESSION_STATE) {
            fprintf(stderr, "[ZK-COMMON] Connection failed with state: %d\n", state);
            return -1;
        }
        
        usleep(100000);  // Sleep 100ms
    }
    
    fprintf(stderr, "[ZK-COMMON] Connection timeout\n");
    return -1;
}

char **zk_get_sorted_chain_nodes(zhandle_t *zh, int *count) {
    if (!zh || !count) return NULL;
    
    struct String_vector children;
    int rc = zoo_get_children(zh, CHAIN_PATH, 0, &children);
    
    if (rc != ZOK) {
        fprintf(stderr, "[ZK-COMMON] Failed to get children: %s\n", zerror(rc));
        *count = 0;
        return NULL;
    }
    
    if (children.count == 0) {
        deallocate_String_vector(&children);
        *count = 0;
        return NULL;
    }
    
    // Allocate array of path strings
    char **node_paths = malloc(children.count * sizeof(char *));
    if (!node_paths) {
        deallocate_String_vector(&children);
        return NULL;
    }
    
    // Build full paths
    for (int i = 0; i < children.count; i++) {
        size_t len = strlen(CHAIN_PATH) + strlen(children.data[i]) + 2;
        node_paths[i] = malloc(len);
        if (!node_paths[i]) {
            // Cleanup on error
            for (int j = 0; j < i; j++) {
                free(node_paths[j]);
            }
            free(node_paths);
            deallocate_String_vector(&children);
            return NULL;
        }
        snprintf(node_paths[i], len, "%s/%s", CHAIN_PATH, children.data[i]);
    }
    
    deallocate_String_vector(&children);
    
    // Sort by path (which sorts by sequence number)
    qsort(node_paths, children.count, sizeof(char *), pathcmp);
    
    *count = children.count;
    return node_paths;
}

void zk_free_node_paths(char **paths, int count) {
    if (!paths) return;
    
    for (int i = 0; i < count; i++) {
        free(paths[i]);
    }
    free(paths);
}

int zk_get_node_address(zhandle_t *zh, const char *node_path, 
                        char *buffer, size_t size) {
    if (!zh || !node_path || !buffer || size == 0) {
        return -1;
    }
    
    int addr_len = size - 1;
    int rc = zoo_get(zh, node_path, 0, buffer, &addr_len, NULL);
    
    if (rc != ZOK) {
        fprintf(stderr, "[ZK-COMMON] Failed to get node data from %s: %s\n", 
                node_path, zerror(rc));
        return -1;
    }
    
    buffer[addr_len] = '\0';
    return 0;
}

int zk_get_node_address_at_index(zhandle_t *zh, char **node_paths, int index,
                                 char *buffer, size_t size) {
    if (!zh || !node_paths || index < 0) {
        return -1;
    }
    
    return zk_get_node_address(zh, node_paths[index], buffer, size);
}

int zk_find_node_index(char **node_paths, int count, const char *target_path) {
    if (!node_paths || !target_path) {
        return -1;
    }
    
    for (int i = 0; i < count; i++) {
        if (strcmp(node_paths[i], target_path) == 0) {
            return i;
        }
    }
    
    return -1;
}