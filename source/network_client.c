/**
 * @file network_client.c
 * 
 * @brief Client-side network communication module
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include "network_client.h"
#include "client_stub-private.h"
#include "message-private.h"
#include "sdmessage.pb-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

struct rlist_t;

static int send_message(int sockfd, MessageT *msg) {
    if (msg == NULL) {
        return -1;
    }
    
    // Get packed size
    size_t msg_size = message_t__get_packed_size(msg);
    if (msg_size > UINT16_MAX) {
        fprintf(stderr, "[ERROR] Message too large\n");
        return -1;
    }
    
    // Allocate and pack
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL) {
        perror("[ERROR] Failed to allocate send buffer");
        return -1;
    }
    
    message_t__pack(msg, buffer);
    
    // Send size
    uint16_t msg_size_net = htons((uint16_t)msg_size);
    if (write_all(sockfd, &msg_size_net, sizeof(uint16_t)) != sizeof(uint16_t)) {
        free(buffer);
        return -1;
    }
    
    // Send message
    int result = write_all(sockfd, buffer, msg_size);
    free(buffer);
    
    return (result == (int)msg_size) ? 0 : -1;
}

static MessageT *receive_message(int sockfd) {
    uint16_t msg_size_net;
    
    // Read size
    if (read_all(sockfd, &msg_size_net, sizeof(uint16_t)) != sizeof(uint16_t)) {
        return NULL;
    }
    
    uint16_t msg_size = ntohs(msg_size_net);
    if (msg_size == 0) {
        fprintf(stderr, "[ERROR] Invalid message size\n");
        return NULL;
    }
    
    // Allocate buffer
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL) {
        perror("[ERROR] Failed to allocate receive buffer");
        return NULL;
    }
    
    // Read message
    if (read_all(sockfd, buffer, msg_size) != msg_size) {
        free(buffer);
        return NULL;
    }
    
    // Unpack
    MessageT *msg = message_t__unpack(NULL, msg_size, buffer);
    free(buffer);
    
    return msg;
}

static int handle_handshake(int sockfd) {
    MessageT *msg = receive_message(sockfd);
    
    if (msg == NULL) {
        fprintf(stderr, "[ERROR] Failed to receive handshake\n");
        return -1;
    }
    
    int opcode = msg->opcode;
    message_t__free_unpacked(msg, NULL);
    
    return opcode;
}

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) com base na
 *   informação guardada na estrutura rlist;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rlist;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rlist_t *rlist) {
    if (rlist == NULL || rlist->server_address == NULL) {
        fprintf(stderr, "[ERROR] Invalid rlist structure\n");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    struct hostent *host;
    
    // Create TCP socket
    rlist->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (rlist->sockfd < 0) {
        perror("[ERROR] Failed to create socket");
        return -1;
    }
    
    // Resolve hostname to IP address
    host = gethostbyname(rlist->server_address);
    if (host == NULL) {
        fprintf(stderr, "[ERROR] Could not resolve hostname: %s\n", 
                rlist->server_address);
        close(rlist->sockfd);
        rlist->sockfd = -1;
        return -1;
    }
    
    // Setup server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(rlist->server_port);
    memcpy(&server_addr.sin_addr.s_addr, 
           host->h_addr_list[0], 
           host->h_length);
    
    // Establish connection to server
    if (connect(rlist->sockfd, 
                (struct sockaddr *)&server_addr, 
                sizeof(server_addr)) < 0) {
        perror("[ERROR] Failed to connect to server");
        close(rlist->sockfd);
        rlist->sockfd = -1;
        return -1;
    }

    // Perform handshake
    switch (handle_handshake(rlist->sockfd))
    {
    case MESSAGE_T__OPCODE__OP_READY:
        printf("Connected to %s:%d\n", 
                rlist->server_address, 
                rlist->server_port);
        return 0;

    case MESSAGE_T__OPCODE__OP_BUSY:
        printf("Server busy. Try again later.\n");
        close(rlist->sockfd);
        rlist->sockfd = -1;
        return -1;
    
    default:
        fprintf(stderr, "[ERROR] Unexpected server response\n");
        close(rlist->sockfd);
        rlist->sockfd = -1;
        return -1;
    }
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rlist_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Tratar de forma apropriada erros de comunicação;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT *network_send_receive(struct rlist_t *rlist, MessageT *msg) {
    if (rlist == NULL || msg == NULL) {
        fprintf(stderr, "[ERROR] Invalid arguments to network_send_receive\n");
        return NULL;
    }
    
    if (rlist->sockfd < 0) {
        fprintf(stderr, "[ERROR] Not connected to server\n");
        return NULL;
    }
    
    /* // Determine message size
    size_t msg_size = message_t__get_packed_size(msg);
    
    if (msg_size > UINT16_MAX) {
        fprintf(stderr, "[ERROR] Message too large (%zu bytes, max %d)\n", 
                msg_size, UINT16_MAX);
        return NULL;
    }
    
    
    // Serialize message to buffer
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL) {
        perror("[ERROR] Failed to allocate serialization buffer");
        return NULL;
    }
    
    size_t packed_size = message_t__pack(msg, buffer);
    if (packed_size != msg_size) {
        fprintf(stderr, "[ERROR] Serialization size mismatch\n");
        free(buffer);
        return NULL;
    }
    
    
    // Send message size (2 bytes, network byte order)
    uint16_t msg_size_net = htons((uint16_t)msg_size);
    if (write_all(rlist->sockfd, &msg_size_net, sizeof(uint16_t)) != sizeof(uint16_t)) {
        perror("[ERROR] Failed to send message size");
        free(buffer);
        return NULL;
    }
    
    
    // Send serialized message
    if (write_all(rlist->sockfd, buffer, msg_size) != (ssize_t)msg_size) {
        perror("[ERROR] Failed to send message");
        free(buffer);
        return NULL;
    }
    
    free(buffer);
    buffer = NULL;
    
    
    // Receive response size (2 bytes, network byte order)
    uint16_t response_size_net;
    if (read_all(rlist->sockfd, &response_size_net, sizeof(uint16_t)) != sizeof(uint16_t)) {
        perror("[ERROR] Failed to receive response size");
        return NULL;
    }
    
    uint16_t response_size = ntohs(response_size_net);
    
    if (response_size == 0) {
        fprintf(stderr, "[ERROR] Invalid response size (0)\n");
        return NULL;
    }
    
    // Receive serialized response
    uint8_t *response_buffer = malloc(response_size);
    if (response_buffer == NULL) {
        perror("[ERROR] Failed to allocate response buffer");
        return NULL;
    }
    
    ssize_t bytes_received = read_all(rlist->sockfd, response_buffer, response_size);
    if (bytes_received != response_size) {
        fprintf(stderr, "[ERROR] Failed to receive complete response (got %zd, expected %u)\n",
                bytes_received, response_size);
        free(response_buffer);
        return NULL;
    }
    

    // Deserialize response
    MessageT *response = message_t__unpack(NULL, response_size, response_buffer);
    free(response_buffer);
    
    if (response == NULL) {
        fprintf(stderr, "[ERROR] Failed to deserialize response\n");
        return NULL;
    }
    
    return response; */

    // Send request
    if (send_message(rlist->sockfd, msg) != 0) {
        perror("[ERROR] Failed to send message");
        return NULL;
    }
    
    // Receive response
    MessageT *response = receive_message(rlist->sockfd);
    if (response == NULL) {
        fprintf(stderr, "[ERROR] Failed to receive response\n");
    }
    
    return response;
}

/* Fecha a ligação estabelecida por network_connect().
 * Retorna 0 (OK) ou -1 (erro).
 */
int network_close(struct rlist_t *rlist) {
    if (rlist == NULL) {
        fprintf(stderr, "network_close: invalid argument (NULL)\n");
        return -1;
    }

    if (rlist->sockfd >= 0) {
        if (close(rlist->sockfd) < 0) {
            perror("close");
            return -1;
        }
        rlist->sockfd = -1;
    }

    return 0;
}
