/**
 * @file network_server.c
 * 
 * @brief Server-side network communication module
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include "network_server.h"
#include "network_server-private.h"
#include "message-private.h"
#include "list_skel.h"
#include "sdmessage.pb-c.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BACKLOG SOMAXCONN

static pthread_mutex_t clients_mutex;
static int client_count = 0;

static FILE *log_fp = NULL;

static volatile sig_atomic_t server_running = 1;
static int listening_socket_global = -1;

/* Client counter management helpers */
static inline int client_connected() {
    pthread_mutex_lock(&clients_mutex);
    int count = ++client_count;
    pthread_mutex_unlock(&clients_mutex);
    return count;
}

static inline int client_disconnected() {
    pthread_mutex_lock(&clients_mutex);
    int count = --client_count;
    pthread_mutex_unlock(&clients_mutex);
    return count;
}

static inline int get_client_count() {
    pthread_mutex_lock(&clients_mutex);
    int count = client_count;
    pthread_mutex_unlock(&clients_mutex);
    return count;
}

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

void *serve_client(void *params) {
    struct client_params *client = (struct client_params *)params;
    struct list_t *list = client->list;

    MessageT *request = NULL;

    while (server_running) {
        request = network_receive(client->socket);
        
        if (request == NULL) {
            break;
        }
        
       if (request != NULL && log_fp) {
            struct ServerLog *l = malloc(sizeof *l);
            if (l) {
                l->tv = malloc(sizeof(struct timeval));
                if (l->tv == NULL) {
                    free(l);
                } else {
                    gettimeofday(l->tv, NULL);

                    char addrstr[64];
                    snprintf(addrstr, sizeof(addrstr), "%s:%d",
                             inet_ntoa(client->address.sin_addr),
                             ntohs(client->address.sin_port));

                    l->client = strdup(addrstr);
                    if (l->client == NULL) {
                        free(l->tv);
                        free(l);
                    } else {
                        l->EventType = REQUEST;
                        l->opcode = request->opcode;
                        l->ctype = request->c_type;
                        l->content = NULL;
                        l->argument = NULL;

                        const char *marca_name = NULL;
                        if (request->data) {
                            switch (request->data->marca) {
                                case MARCA__MARCA_TOYOTA:   marca_name = "Toyota"; break;
                                case MARCA__MARCA_BMW:      marca_name = "BMW"; break;
                                case MARCA__MARCA_RENAULT:  marca_name = "Renault"; break;
                                case MARCA__MARCA_AUDI:     marca_name = "Audi"; break;
                                case MARCA__MARCA_MERCEDES: marca_name = "Mercedes"; break;
                                default:                    marca_name = "Unknown"; break;
                            }
                        }

                        if (request->c_type == MESSAGE_T__C_TYPE__CT_DATA && request->data) {
                            l->argument = malloc(sizeof(char*) * 4);
                            if (l->argument) {
                                l->argument[0] = strdup(marca_name ? marca_name : "");
                                l->argument[1] = strdup(request->data->modelo ? request->data->modelo : "");
                                char tmp_ano[32];
                                snprintf(tmp_ano, sizeof(tmp_ano), "%d", request->data->ano);
                                l->argument[2] = strdup(tmp_ano);
                                l->argument[3] = NULL;

                                if (!l->argument[0] || !l->argument[1] || !l->argument[2]) {
                                    if (l->argument[0]) free(l->argument[0]);
                                    if (l->argument[1]) free(l->argument[1]);
                                    if (l->argument[2]) free(l->argument[2]);
                                    free(l->argument);
                                    l->argument = NULL;
                                }
                            }
                            if (l->content == NULL) {
                                l->content = strdup("");
                            }
                        } else if (request->c_type == MESSAGE_T__C_TYPE__CT_MODEL && request->n_models > 0 && request->models[0]) {
                            l->content = strdup(request->models[0]);
                            l->argument = NULL;
                        } else if (request->c_type == MESSAGE_T__C_TYPE__CT_NONE) {
                            l->content = strdup("");
                            l->argument = NULL;
                        } else {
                            if (request->data && request->data->modelo) {
                                l->content = strdup(request->data->modelo);
                            } else if (request->n_models > 0 && request->models[0]) {
                                l->content = strdup(request->models[0]);
                            } else {
                                l->content = strdup("");
                            }
                            l->argument = NULL;
                        }

                        WriteLog(l, log_fp);
                        free(l);
                    }
                }
            }
        }
        
        // Process request
        if (invoke(request, list) != 0) {
            fprintf(stderr, "[ERROR] Failed to process request\n");
            message_t__free_unpacked(request, NULL);
            break;
        }
        
        if (network_send(client->socket, request) != 0) {
            fprintf(stderr, "[ERROR] Failed to send response\n");
            message_t__free_unpacked(request, NULL);
            break;
        }
        
        message_t__free_unpacked(request, NULL);
        request = NULL;
    }

    // Cleanup
    if (request != NULL) {
        message_t__free_unpacked(request, NULL);
    }

    if (log_fp) {
    struct ServerLog *l = malloc(sizeof *l);
    if (l) {
        l->tv = malloc(sizeof(struct timeval));
        if (l->tv == NULL) {
            free(l);
        } else {
            gettimeofday(l->tv, NULL);
            char addrstr[64];
            snprintf(addrstr, sizeof(addrstr), "%s:%d",
                     inet_ntoa(client->address.sin_addr),
                     ntohs(client->address.sin_port));
            l->client = strdup(addrstr);
            if (l->client == NULL) {
                free(l->tv);
                free(l);
            } else {
                l->EventType = CLOSE;
                l->opcode = MESSAGE_T__OPCODE__OP_BAD;
                l->ctype = MESSAGE_T__C_TYPE__CT_NONE;
                l->content = NULL;
                l->argument = NULL;
                WriteLog(l, log_fp);
                free(l);
            }
        }
    }
}

    //  Terminate client connection
    close(client->socket);
    //  Update active client count
    client_disconnected();
    printf("[SERVER] Connection closed (Active: %d)\n", get_client_count());
    //  Free client parameters
    free(client);

    return NULL;
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

    // Initialize client counter mutex
    pthread_mutex_init(&clients_mutex, NULL);

    log_fp = CreateFile();
    if(!log_fp){
        fprintf(stderr, "[ERROR] Could not open server.log\n");
        pthread_mutex_destroy(&clients_mutex);
        close (server_socket);
        return -1;
    }
    
    return server_socket;
}

static struct client_params *init_params(int socket, struct sockaddr_in *address, struct list_t *list) {
    struct client_params *params = malloc(sizeof *params);
    if (params == NULL) return NULL;

    params->socket = socket;
    params->address = *address;
    params->list = list;
    return params;
}

static int send_status(int client_socket, MessageT__Opcode opcode) {
    MessageT handshake_msg = MESSAGE_T__INIT;
    handshake_msg.opcode = opcode;
    handshake_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    return network_send(client_socket, &handshake_msg);
}

int network_main_loop(int listening_socket, struct list_t *list) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_socket;
    
    if (list == NULL) {
        fprintf(stderr, "[ERROR] List not initialized\n");
        return -1;
    }
    
    // Set global listening socket for use in signal handlers
    listening_socket_global = listening_socket;

    while (server_running) {
        client_addr_len = sizeof(client_addr);
        
        client_socket = accept(listening_socket, 
                              (struct sockaddr *)&client_addr, 
                              &client_addr_len);
        
        if (client_socket < 0) {
            if (errno == EINTR) continue;
            if (!server_running) break;
            perror("[ERROR] Failed to accept connection");
            return -1;
        }

        if (get_client_count() >= MAX_CLIENTS) {
            printf("[SERVER] Max clients reached. Rejecting: %s:%d\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));

            // Send OP_BUSY message
            send_status(client_socket, MESSAGE_T__OPCODE__OP_BUSY);
            close(client_socket);
            continue;
        }

        // Send OP_READY message
        if (send_status(client_socket, MESSAGE_T__OPCODE__OP_READY) != 0) {
            fprintf(stderr, "[ERROR] Failed to send READY message\n");
            close(client_socket);
            continue;
        }

        // Increment client counter
        int count = client_connected();

        if(log_fp){
            struct ServerLog *l = malloc(sizeof *l);
            if(l){
                l->tv = malloc(sizeof (struct timeval));
                if(l->tv == NULL){
                    free(l);
                } else{
                    gettimeofday(l->tv, NULL);

                    char addrstr[64];
                    snprintf(addrstr, sizeof(addrstr), "%s:%d",
                            inet_ntoa(client_addr.sin_addr),
                            ntohs(client_addr.sin_port));
                    
                    l->client = strdup(addrstr);
                    if(l->client == NULL){
                        free(l->tv);
                        free(l);
                    } else{
                        l->EventType = CONNECT;
                        l->opcode = MESSAGE_T__OPCODE__OP_BAD; 
                        l->ctype = MESSAGE_T__C_TYPE__CT_NONE;
                        l->content = NULL;
                        l->argument = NULL;

                        WriteLog(l, log_fp);
                        free(l);
                    }
                }
            }
        }

printf("[SERVER] Connection accepted (Active: %d)\n", count);

        struct client_params *client_params = init_params(client_socket, &client_addr, list);
        if (client_params == NULL) {
            fprintf(stderr, "[ERROR] Failed to allocate client thread parameters\n");
            close(client_socket);
            client_disconnected();
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, serve_client, client_params) != 0) {
            fprintf(stderr, "[ERROR] Failed to create thread\n");
            close(client_socket);
            free(client_params);
            client_disconnected();
            continue;
        }
        
        pthread_detach(thread_id);
    }
    listening_socket_global = -1;
    return 0;
}

int network_server_close(int socket) {

    if(log_fp){
        CloseFile(log_fp);
        log_fp=NULL;
    }
    // Destroy client counter mutex
    pthread_mutex_destroy(&clients_mutex);

    if (socket < 0) {
        return -1;
    }
    
    return close(socket);
}

void network_server_request_shutdown() {
    // Signal the server to stop running
    server_running = 0;

     // Close listening socket to unblock accept()
    if (listening_socket_global >= 0) {
        shutdown(listening_socket_global, SHUT_RDWR);
        close(listening_socket_global);
    }
}
