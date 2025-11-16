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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static int socket = -1;
static struct list_t *list = NULL;

static void signal_handler(int signum) {
    printf("[SERVER] Signal %d received, shutting down...\n", signum);
    
    network_server_request_shutdown();
}

static void cleanup(void) {
    printf("[SERVER] Cleaning up resources...\n");

    if (socket >= 0) {
        network_server_close(socket);
        socket = -1;
    }
    
    if (list != NULL) {
        list_skel_destroy(list);
        list = NULL;
    }
    
    printf("[SERVER] Shutdown complete\n");
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Parse port number
    int port = atoi(argv[1]);
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "[ERROR] Invalid port: %d (must be 1024-65535)\n", port);
        return 1;
    }

    printf("[SERVER] Port= %d\n", port);

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

    // Initialize server
    printf("[SERVER] Initializing network on port %d...\n", port);
    socket = network_server_init(port);
    if (socket < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize server\n");
        return 1;
    }
    printf("[SERVER] Server ready (socket fd=%d)\n\n", socket);

    // Main loop
    printf("[SERVER] Waiting for connections...\n");
    if (network_main_loop(socket, list) < 0) {
        fprintf(stderr, "[ERROR] Main loop failed\n");
        return 1;
    }

    return 0;
}