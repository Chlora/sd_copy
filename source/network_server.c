/**
 * @file network_server.c
 * 
 * @brief
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include "network_server.h"
#include "message-private.h"
#include "list_skel.h"
#include "sdmessage.pb-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BACKLOG 5

static volatile sig_atomic_t server_running = 1;

MessageT *network_receive(int client_socket) {
    uint16_t msg_size_net;
    if (read_all(client_socket, &msg_size_net, sizeof(uint16_t)) != sizeof(uint16_t)) {
        return NULL;
    }
    
    uint16_t msg_size = ntohs(msg_size_net);
    
    if (msg_size == 0) {
        fprintf(stderr, "[ERROR] Invalid message size (0)\n");
        return NULL;
    }
    
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL) {
        perror("[ERROR] Failed to allocate message buffer");
        return NULL;
    }
    
    if (read_all(client_socket, buffer, msg_size) != msg_size) {
        free(buffer);
        return NULL;
    }
    
    MessageT *msg = message_t__unpack(NULL, msg_size, buffer);
    free(buffer);
    
    if (msg == NULL) {
        fprintf(stderr, "[ERROR] Failed to deserialize message\n");
    }
    
    return msg;
}

int network_send(int client_socket, MessageT *msg) {
    if (msg == NULL) {
        return -1;
    }
    
    size_t msg_size = message_t__get_packed_size(msg);
    
    if (msg_size > UINT16_MAX) {
        fprintf(stderr, "[ERROR] Message too large (%zu bytes)\n", msg_size);
        return -1;
    }
    
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL) {
        perror("[ERROR] Failed to allocate serialization buffer");
        return -1;
    }
    
    message_t__pack(msg, buffer);
    
    uint16_t msg_size_net = htons((uint16_t)msg_size);
    if (write_all(client_socket, &msg_size_net, sizeof(uint16_t)) != sizeof(uint16_t)) {
        free(buffer);
        return -1;
    }
    
    int result = write_all(client_socket, buffer, msg_size);
    free(buffer);
    
    return (result == (int)msg_size) ? 0 : -1;
}

static int handle_client(int client_socket, struct list_t *list) {
    MessageT *request = NULL;
    int error = 0;
    
    while (server_running && !error) {
        request = network_receive(client_socket);
        
        if (request == NULL) {
            break;
        }
        
        if (invoke(request, list) != 0) {
            fprintf(stderr, "[ERROR] Failed to process request\n");
            error = 1;
        }
        
        if (network_send(client_socket, request) != 0) {
            fprintf(stderr, "[ERROR] Failed to send response\n");
            error = 1;
        }
        
        message_t__free_unpacked(request, NULL);
        request = NULL;
    }
    
    if (request != NULL) {
        message_t__free_unpacked(request, NULL);
    }
    
    close(client_socket);
    return error ? -1 : 0;
}

int network_server_init(short port) {
    int server_socket;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("[ERROR] Failed to create socket");
        return -1;
    }
    
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[ERROR] Failed to set SO_REUSEADDR");
        close(server_socket);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[ERROR] Failed to bind socket");
        close(server_socket);
        return -1;
    }
    
    if (listen(server_socket, BACKLOG) < 0) {
        perror("[ERROR] Failed to listen on socket");
        close(server_socket);
        return -1;
    }
    
    return server_socket;
}

int network_main_loop(int listening_socket, struct list_t *list) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_socket;
    
    if (list == NULL) {
        fprintf(stderr, "[ERROR] List not initialized\n");
        return -1;
    }
    
    while (server_running) {
        client_addr_len = sizeof(client_addr);
        
        client_socket = accept(listening_socket, 
                              (struct sockaddr *)&client_addr, 
                              &client_addr_len);
        
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("[ERROR] Failed to accept connection");
            return -1;
        }
        
        printf("[SERVER] Client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        
        handle_client(client_socket, list);

        printf("[SERVER] Client disconnected\n");
    }
    
    return 0;
}

int network_server_close(int socket) {
    if (socket < 0) {
        return -1;
    }
    
    return close(socket);
}

void network_server_request_shutdown(void) {
    server_running = 0;
}
