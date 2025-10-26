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

/* static ssize_t send_all(int sockfd, const void *buf, size_t len) {
    size_t total = 0;
    const uint8_t *p = buf;

    while (total < len) {
        ssize_t n = send(sockfd, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue; // retry
            return -1;
        }
        if (n == 0) {
            // fechou ligacao
            return -1;
        }
        total += (size_t)n;
    }
    return (ssize_t)total;
}

static ssize_t recv_all(int sockfd, void *buf, size_t len) {
    size_t total = 0;
    uint8_t *p = buf;

    while (total < len) {
        ssize_t n = recv(sockfd, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) {
            // interrupcao
            return -1;
        }
        total += (size_t)n;
    }
    return (ssize_t)total;
} */

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) com base na
 *   informação guardada na estrutura rlist;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rlist;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rlist_t *rlist) {

    /* char portbuf[16];
    snprintf(portbuf, sizeof portbuf, "%d", rlist->server_port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL, *ai = NULL;
    int rc = getaddrinfo(rlist->server_address, portbuf, &hints, &res);
    if (rc != 0) {
        return -1;
    }

    int fd = -1;
    for (ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;

        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
            rlist->sockfd = fd;
            freeaddrinfo(res);
            return 0;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return -1; */

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
    
    printf("[CLIENT] Connected to %s:%d (socket fd=%d)\n", 
           rlist->server_address, 
           rlist->server_port,
           rlist->sockfd);
    
    return 0;
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
    /* if (rlist == NULL || msg == NULL) {
        fprintf(stderr, "network_send_receive: invalid arguments\n");
        return NULL;
    }

    int sockfd = rlist->sockfd;
    if (sockfd < 0) {
        fprintf(stderr, "network_send_receive: invalid socket\n");
        return NULL;
    }

    size_t msg_size = message_t__get_packed_size(msg);
    uint8_t *msg_buf = malloc(msg_size);
    if (msg_buf == NULL) {
        perror("malloc");
        return NULL;
    }

    message_t__pack(msg, msg_buf);

    uint32_t net_len = htonl((uint32_t)msg_size);

    if (send_all(sockfd, &net_len, sizeof(net_len)) < 0) {
        perror("send_all length");
        free(msg_buf);
        return NULL;
    }

    if (send_all(sockfd, msg_buf, msg_size) < 0) {
        perror("send_all payload");
        free(msg_buf);
        return NULL;
    }

    free(msg_buf); 


    uint32_t reply_len_net;
    if (recv_all(sockfd, &reply_len_net, sizeof(reply_len_net)) < 0) {
        perror("recv_all length");
        return NULL;
    }

    uint32_t reply_len = ntohl(reply_len_net);

    if (reply_len == 0) {
        fprintf(stderr, "network_send_receive: server replied with 0-length message\n");
        return NULL;
    }

    uint8_t *reply_buf = malloc(reply_len);
    if (reply_buf == NULL) {
        perror("malloc reply_buf");
        return NULL;
    }

    if (recv_all(sockfd, reply_buf, reply_len) < 0) {
        perror("recv_all payload");
        free(reply_buf);
        return NULL;
    }

    MessageT *reply_msg = message_t__unpack(NULL, reply_len, reply_buf);
    free(reply_buf);

    if (reply_msg == NULL) {
        fprintf(stderr, "message_t__unpack: failed to decode server reply\n");
        return NULL;
    }

    return reply_msg; */

    if (rlist == NULL || msg == NULL) {
        fprintf(stderr, "[ERROR] Invalid arguments to network_send_receive\n");
        return NULL;
    }
    
    if (rlist->sockfd < 0) {
        fprintf(stderr, "[ERROR] Not connected to server\n");
        return NULL;
    }
    
    
    // Determine message size
    
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
