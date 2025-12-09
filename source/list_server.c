/**
 * @file list_server.c
 * 
 * @brief Server-side implementation for list operations
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include "list_skel.h"
#include "network_server.h"
#include "zk_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static int s_socket = -1;
static struct list_t *list = NULL;

static int build_local_address(char* buffer, size_t buffer_size, const char* port) {
    if (!buffer || !port || buffer_size < 11 + strlen(port) + 1) {
        return -1;
    }
    snprintf(buffer, buffer_size, "127.0.0.1:%s", port);
    return 0;
}

static void signal_handler(int signum) {
    printf("[SERVER] Signal %d received, shutting down...\n", signum);
    
    network_server_request_shutdown();
}

static void cleanup(void) {
    printf("[SERVER] Cleaning up resources...\n");

    // Sleep to allow threads to complete their cleanup
    usleep(200000);  // 200ms

    // Disconnect from ZooKeeper
    zk_disconnect();

    if (s_socket >= 0) {
        network_server_close(s_socket);
        s_socket = -1;
    }

    if (list != NULL) {
        list_skel_destroy(list);
        list = NULL;
    }

    printf("[SERVER] Shutdown complete\n");
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <zk_host:zk_port>\n", argv[0]);
        return 1;
    }

    // Parse port number
    int port = atoi(argv[1]);
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "[ERROR] Invalid port: %d (must be 1024-65535)\n", port);
        return 1;
    }

    const char *zk_host = argv[2];
    char server_addr[256];
    if (build_local_address(server_addr, sizeof(server_addr), argv[1]) != 0) {
        fprintf(stderr, "[ERROR] Failed to build server address\n");
        return 1;
    }

    // Setup signal handlers
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGPIPE, SIG_IGN);         // Ignore SIGPIPE

    // Register cleanup function at exit
    atexit(cleanup);

    // Initialize list skeleton
    printf("[SERVER] Initializing list...\n");
    list = list_skel_init();
    if (list == NULL) {
        fprintf(stderr, "[ERROR] Failed to initialize list\n");
        return 1;
    }
    printf("[SERVER] List initialized successfully\n");

    // Initialize zookeeper
    if (zk_connect(zk_host, server_addr) != 0) {
        fprintf(stderr, "[ERROR] Failed to connect to ZooKeeper\n");
        return 1;
    }

    if (zk_register() != 0) {
        fprintf(stderr, "[ERROR] Failed to register in ZooKeeper\n");
        return 1;
    }

    if (zk_update_chain() != 0) {
        fprintf(stderr, "[ERROR] Failed to update chain\n");
        return 1;
    }
    
    // Sync from predecessor
    if (zk_sync(list) != 0) {
        fprintf(stderr, "[ERROR] Failed to sync from predecessor\n");
        return 1;
    }
    
    zk_print_status();

    // Initialize server
    printf("[SERVER] Initializing network on port %d...\n", port);
    s_socket = network_server_init(port);
    if (s_socket < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize server\n");
        return 1;
    }
    printf("[SERVER] Server ready (s_socket fd=%d)\n\n", s_socket);

    // Main loop
    printf("[SERVER] Waiting for connections...\n");
    if (network_main_loop(s_socket, list) < 0) {
        fprintf(stderr, "[ERROR] Main loop failed\n");
        return 1;
    }

    return 0;
}