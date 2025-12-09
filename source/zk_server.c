/**
 * @file zk_server.c
 * @brief ZooKeeper server-side implementation
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#define THREADED

#include "zk_server.h"
#include "zk_util.h"
#include "client_stub.h"
#include "client_stub-private.h"
#include "network_client.h"
#include "list.h"
#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#define NODE_PREFIX "/chain/node"

/* Zookeeper State */
typedef struct {
    // ZooKeeper connection
    zhandle_t *handle;
    char *node_path;
    char *node_address;
    
    // Chain topology
    struct rlist_t *successor;
    char *successor_address;
    char *predecessor_address;
    zk_role_t role;
    int node_index;
    int chain_size;
    
    // Synchronization
    pthread_mutex_t zk_mutex;
    pthread_cond_t connected_cond;
    int mutex_initialized;
    int cond_initialized;
    volatile int connected;
    volatile int update_pending;
    volatile int active;
} zk_state_t;

static zk_state_t zk_state = {0};

/* Forward Declarations */
static void connection_watcher(zhandle_t *zh, int type, int state, 
                               const char *path, void *context);
static void chain_watcher(zhandle_t *zh, int type, int state,
                         const char *path, void *context);
static int zk_update_chain_internal();


/* Utilities */
static void cleanup_zk_state() {
    if (zk_state.handle) {
        zookeeper_close(zk_state.handle);
        zk_state.handle = NULL;
    }
    
    if (zk_state.node_path) {
        free(zk_state.node_path);
        zk_state.node_path = NULL;
    }
    
    if (zk_state.node_address) {
        free(zk_state.node_address);
        zk_state.node_address = NULL;
    }
    
    if (zk_state.successor_address) {
        free(zk_state.successor_address);
        zk_state.successor_address = NULL;
    }
    
    if (zk_state.predecessor_address) {
        free(zk_state.predecessor_address);
        zk_state.predecessor_address = NULL;
    }
    
    if (zk_state.cond_initialized) {
        pthread_cond_destroy(&zk_state.connected_cond);
        zk_state.cond_initialized = 0;
    }
    
    if (zk_state.mutex_initialized) {
        pthread_mutex_destroy(&zk_state.zk_mutex);
        zk_state.mutex_initialized = 0;
    }
}

static int wait_for_connection(int timeout_sec) {
    struct timespec timeout;
    timeout.tv_sec = time(NULL) + timeout_sec;
    timeout.tv_nsec = 0;
    
    pthread_mutex_lock(&zk_state.zk_mutex);
    
    while (!zk_state.connected) {
        int rc = pthread_cond_timedwait(&zk_state.connected_cond,
                                        &zk_state.zk_mutex,
                                        &timeout);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&zk_state.zk_mutex);
            fprintf(stderr, "[ZK] Connection timeout\n");
            return -1;
        }
    }
    
    pthread_mutex_unlock(&zk_state.zk_mutex);
    return 0;
}

/* Update Thread */
static void *zk_update_thread(void *arg) {
    (void)arg;
    
    while (zk_state.active) {
        pthread_mutex_lock(&zk_state.zk_mutex);
        
        while (!zk_state.update_pending && zk_state.active) {
            pthread_cond_wait(&zk_state.connected_cond, &zk_state.zk_mutex);
        }
        
        if (!zk_state.active) {
            pthread_mutex_unlock(&zk_state.zk_mutex);
            break;
        }
        
        zk_state.update_pending = 0;
        pthread_mutex_unlock(&zk_state.zk_mutex);
        
        printf("[ZK-THREAD] Processing chain update\n");
        zk_update_chain_internal();
    }
    
    return NULL;
}

/* Zookeeper Operations */

int zk_connect(const char *zk_host, const char *node_addr) {
    if (!zk_host || !node_addr) {
        fprintf(stderr, "[ZK] Invalid arguments\n");
        return -1;
    }
    
    printf("[ZK] Initializing for %s, connecting to %s\n", node_addr, zk_host);
    
    memset(&zk_state, 0, sizeof(zk_state));
    zk_state.active = 1;
    zk_state.role = ZK_ROLE_SINGLE;
    zk_state.connected = 0;
    zk_state.node_index = -1;
    zk_state.chain_size = 0;
    
    if (pthread_mutex_init(&zk_state.zk_mutex, NULL) != 0) {
        perror("[ZK] pthread_mutex_init");
        return -1;
    }
    zk_state.mutex_initialized = 1;
    
    if (pthread_cond_init(&zk_state.connected_cond, NULL) != 0) {
        perror("[ZK] pthread_cond_init");
        cleanup_zk_state();
        return -1;
    }
    zk_state.cond_initialized = 1;
    
    zk_state.node_address = strdup(node_addr);
    if (!zk_state.node_address) {
        perror("[ZK] strdup");
        cleanup_zk_state();
        return -1;
    }
    
    printf("[ZK] Connecting to ZooKeeper...\n");
    zk_state.handle = zookeeper_init(zk_host, connection_watcher, TIMEOUT_MS, 
                                    NULL, &zk_state, 0);
    if (!zk_state.handle) {
        fprintf(stderr, "[ZK] zookeeper_init failed\n");
        cleanup_zk_state();
        return -1;
    }
    
    if (wait_for_connection(CONNECT_TIMEOUT_SEC) != 0) {
        cleanup_zk_state();
        return -1;
    }
    
    printf("[ZK] Connected successfully\n");
    
    // Start update thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, zk_update_thread, NULL) != 0) {
        perror("[ZK] pthread_create");
        cleanup_zk_state();
        return -1;
    }
    pthread_detach(thread);
    
    return 0;
}

int zk_register() {
    if (!zk_state.handle) {
        fprintf(stderr, "[ZK] Invalid state\n");
        return -1;
    }
    
    printf("[ZK] Registering server...\n");
    
    int rc = zoo_create(zk_state.handle, CHAIN_PATH, "", 0,
                       &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    
    if (rc != ZOK && rc != ZNODEEXISTS) {
        fprintf(stderr, "[ZK] Failed to create %s: %s\n", CHAIN_PATH, zerror(rc));
        return -1;
    }
    
    if (rc == ZOK) {
        printf("[ZK] Created %s node\n", CHAIN_PATH);
    }
    
    char path_buffer[256];
    rc = zoo_create(zk_state.handle, NODE_PREFIX,
                   zk_state.node_address, strlen(zk_state.node_address),
                   &ZOO_OPEN_ACL_UNSAFE,
                   ZOO_EPHEMERAL | ZOO_SEQUENCE,
                   path_buffer, sizeof(path_buffer));
    
    if (rc != ZOK) {
        fprintf(stderr, "[ZK] Failed to create node: %s\n", zerror(rc));
        return -1;
    }
    
    zk_state.node_path = strdup(path_buffer);
    if (!zk_state.node_path) {
        perror("[ZK] strdup");
        return -1;
    }
    
    printf("[ZK] Registered as %s\n", path_buffer);
    return 0;
}

static int zk_update_chain_internal() {
    if (!zk_state.handle || !zk_state.node_path) {
        fprintf(stderr, "[ZK] Invalid state\n");
        return -1;
    }
    
    printf("[ZK] Updating chain...\n");
    
    // Set watcher
    struct String_vector children;
    int rc = zoo_wget_children(zk_state.handle, CHAIN_PATH, chain_watcher, 
                               &zk_state, &children);
    if (rc != ZOK) {
        fprintf(stderr, "[ZK] Failed to get children: %s\n", zerror(rc));
        return -1;
    }
    deallocate_String_vector(&children);
    
    // Get sorted nodes
    int count;
    char **node_paths = zk_get_sorted_chain_nodes(zk_state.handle, &count);
    if (!node_paths || count == 0) {
        fprintf(stderr, "[ZK] No nodes in chain!\n");
        return -1;
    }
    
    // Find our position
    int node_index = zk_find_node_index(node_paths, count, zk_state.node_path);
    if (node_index == -1) {
        fprintf(stderr, "[ZK] Could not find ourselves in chain!\n");
        zk_free_node_paths(node_paths, count);
        return -1;
    }
    
    printf("[ZK] Chain: %d servers, position: %d\n", count, node_index + 1);
    
    // Determine role
    zk_role_t new_role;
    if (count == 1) {
        new_role = ZK_ROLE_SINGLE;
    } else if (node_index == 0) {
        new_role = ZK_ROLE_HEAD;
    } else if (node_index == count - 1) {
        new_role = ZK_ROLE_TAIL;
    } else {
        new_role = ZK_ROLE_MIDDLE;
    }
    
    // Get successor if we have one
    char *new_successor_addr = NULL;
    struct rlist_t *new_successor = NULL;

    if (node_index < count - 1) {
        char addr_buffer[256];
        if (zk_get_node_address_at_index(zk_state.handle, node_paths, node_index + 1,
                                       addr_buffer, sizeof(addr_buffer)) == 0) {
            new_successor_addr = strdup(addr_buffer);
            printf("[ZK] Connecting to successor: %s\n", addr_buffer);

            // Retry connection with exponential backoff
            int max_retries = 5;
            int delay_ms = 100;

            for (int retry = 0; retry < max_retries; retry++) {
                new_successor = rlist_connect(addr_buffer);
                if (new_successor) {
                    printf("[ZK] Successfully connected to successor\n");
                    break;
                }

                if (retry < max_retries - 1) {
                    printf("[ZK] Connection attempt %d failed, retrying in %dms...\n",
                           retry + 1, delay_ms);
                    usleep(delay_ms * 1000);
                    delay_ms *= 2;  // Exponential backoff
                }
            }

            if (!new_successor) {
                fprintf(stderr, "[ZK] Failed to connect to successor after %d attempts\n",
                        max_retries);
                free(new_successor_addr);
                new_successor_addr = NULL;
            }
        }
    }
    
    // Get predecessor if we have one
    char *new_predecessor_addr = NULL;
    
    if (node_index > 0) {
        char addr_buffer[256];
        if (zk_get_node_address_at_index(zk_state.handle, node_paths, node_index - 1, 
                                       addr_buffer, sizeof(addr_buffer)) == 0) {
            new_predecessor_addr = strdup(addr_buffer);
            printf("[ZK] Predecessor: %s\n", addr_buffer);
        }
    }
    
    zk_free_node_paths(node_paths, count);
    
    // Update state atomically
    pthread_mutex_lock(&zk_state.zk_mutex);
    
    struct rlist_t *old_successor = zk_state.successor;
    char *old_successor_addr = zk_state.successor_address;
    char *old_predecessor_addr = zk_state.predecessor_address;
    
    zk_state.successor = new_successor;
    zk_state.successor_address = new_successor_addr;
    zk_state.predecessor_address = new_predecessor_addr;
    zk_state.role = new_role;
    zk_state.node_index = node_index;
    zk_state.chain_size = count;
    
    pthread_mutex_unlock(&zk_state.zk_mutex);
    
    // Cleanup old
    if (old_successor && old_successor != new_successor) {
        printf("[ZK] Disconnecting from old successor\n");
        rlist_disconnect(old_successor);
    }
    if (old_successor_addr) free(old_successor_addr);
    if (old_predecessor_addr) free(old_predecessor_addr);
    
    printf("[ZK] Role: %s\n", zk_role_string(new_role));
    return 0;
}

int zk_update_chain() {
    return zk_update_chain_internal();
}

int zk_forward(MessageT *msg) {
    if (!msg) return -1;
    
    for (int attempt = 0; attempt < 2; attempt++) {
        pthread_mutex_lock(&zk_state.zk_mutex);
        struct rlist_t *successor = zk_state.successor;
        int has_successor = (zk_state.node_index < zk_state.chain_size - 1);
        pthread_mutex_unlock(&zk_state.zk_mutex);
        
        // No forwarding needed if we're last in chain
        if (!has_successor) {
            return 0;
        }
        
        if (!successor) {
            fprintf(stderr, "[ZK] No successor connection\n");
            return -1;
        }
        
        // Try forward
        MessageT *reply = network_send_receive(successor, msg);
        
        if (reply->opcode == msg->opcode + 1) {
            message_t__free_unpacked(reply, NULL);
            return 0;
        }
        
        if (attempt == 0) {
            printf("[ZK] Forward failed, waiting for topology update...\n");
            usleep(50000);
        }
    }
    
    fprintf(stderr, "[ZK] Forward failed after retry\n");
    return -1;
}

int zk_sync(struct list_t *list) {
    if (!list) {
        fprintf(stderr, "[ZK-SYNC] Invalid list\n");
        return -1;
    }
    
    pthread_mutex_lock(&zk_state.zk_mutex);
    char *pred_addr = zk_state.predecessor_address;
    if (pred_addr) pred_addr = strdup(pred_addr);
    pthread_mutex_unlock(&zk_state.zk_mutex);
    
    if (!pred_addr) {
        printf("[ZK-SYNC] No predecessor (head/single)\n");
        pthread_mutex_lock(&zk_state.zk_mutex);
        pthread_mutex_unlock(&zk_state.zk_mutex);
        return 0;
    }
    
    printf("[ZK-SYNC] Connecting to predecessor: %s\n", pred_addr);
    
    struct rlist_t *pred = rlist_connect(pred_addr);
    free(pred_addr);
    
    if (!pred) {
        fprintf(stderr, "[ZK-SYNC] Connection failed\n");
        return -1;
    }
    
    // Sync all cars
    int sync_count = 0;
    struct data_t **cars = rlist_get_all(pred);
    if (cars) {
        for (int i = 0; cars[i]; i++) {
            struct data_t *copy = data_dup(cars[i]);
            if (copy && list_add(list, copy) == 0) {
                sync_count++;
            } else if (copy) {
                data_destroy(copy);
            }
            data_destroy(cars[i]);
        }
        free(cars);
    }
    
    rlist_disconnect(pred);
    
    printf("[ZK-SYNC] Synced %d cars\n", sync_count);
    return 0;
}

void zk_disconnect() {
    printf("[ZK] Cleaning up...\n");

    // Signal update thread to stop
    if (zk_state.mutex_initialized) {
        pthread_mutex_lock(&zk_state.zk_mutex);
        zk_state.active = 0;
        pthread_cond_broadcast(&zk_state.connected_cond);  // Wake up the thread
        pthread_mutex_unlock(&zk_state.zk_mutex);
    }

    // Give thread time to exit
    usleep(100000);  // 100ms

    if (zk_state.mutex_initialized && zk_state.successor) {
        pthread_mutex_lock(&zk_state.zk_mutex);
        rlist_disconnect(zk_state.successor);
        zk_state.successor = NULL;
        pthread_mutex_unlock(&zk_state.zk_mutex);
    }

    cleanup_zk_state();
    printf("[ZK] Cleanup complete\n");
}

void zk_print_status() {
    pthread_mutex_lock(&zk_state.zk_mutex);
    
    printf("\n- ZooKeeper Status -\n");
    printf("\tNode: %s\n", zk_state.node_address ? zk_state.node_address : "N/A");
    printf("\tPosition: %d/%d\n", zk_state.node_index + 1, zk_state.chain_size);
    printf("\tRole: %s\n", zk_role_string(zk_state.role));
    printf("\tPredecessor: %s\n", zk_state.predecessor_address ? zk_state.predecessor_address : "NONE");
    printf("\tSuccessor: %s\n", zk_state.successor_address ? zk_state.successor_address : "NONE");
    printf("\n\n");
    
    pthread_mutex_unlock(&zk_state.zk_mutex);
}

const char *zk_role_string(zk_role_t role) {
    switch (role) {
        case ZK_ROLE_SINGLE: return "SINGLE";
        case ZK_ROLE_HEAD:   return "HEAD";
        case ZK_ROLE_MIDDLE: return "MIDDLE";
        case ZK_ROLE_TAIL:   return "TAIL";
        default:             return "UNKNOWN";
    }
}

int zk_get_chain_position() {
    pthread_mutex_lock(&zk_state.zk_mutex);
    int pos = zk_state.node_index + 1;
    pthread_mutex_unlock(&zk_state.zk_mutex);
    return pos;
}

int zk_get_chain_size() {
    pthread_mutex_lock(&zk_state.zk_mutex);
    int size = zk_state.chain_size;
    pthread_mutex_unlock(&zk_state.zk_mutex);
    return size;
}

/* Watchers */

static void connection_watcher(zhandle_t *zh, int type, int state,
                               const char *path, void *context) {
    (void)zh;
    (void)path;
                                
    zk_state_t *server = (zk_state_t *)context;
    
    if (!server) return;
    
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            printf("[ZK] Session connected\n");
            
            pthread_mutex_lock(&server->zk_mutex);
            server->connected = 1;
            pthread_cond_signal(&server->connected_cond);
            pthread_mutex_unlock(&server->zk_mutex);
            
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            fprintf(stderr, "[ZK] Session expired!\n");
            
            pthread_mutex_lock(&server->zk_mutex);
            server->connected = 0;
            server->active = 0;
            pthread_mutex_unlock(&server->zk_mutex);
            
        } else if (state == ZOO_CONNECTING_STATE) {
            printf("[ZK] Reconnecting...\n");
        }
    }
}

static void chain_watcher(zhandle_t *zh, int type, int state,
                         const char *path, void *context) {

    (void)zh;
    (void)state;
    (void)path;
    
    zk_state_t *server = (zk_state_t *)context;
    
    if (!server) return;
    
    if (type == ZOO_CHILD_EVENT) {
        printf("[ZK] Chain membership changed!\n");
        
        pthread_mutex_lock(&server->zk_mutex);
        server->update_pending = 1;
        pthread_cond_signal(&server->connected_cond);
        pthread_mutex_unlock(&server->zk_mutex);
    }
}