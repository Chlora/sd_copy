#include "client_stub.h"
#include "sdmessage.pb-c.h"
#include "client_stub-private.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <errno.h>
#include <stdint.h>


struct rlist_t;

static ssize_t send_all(int sockfd, const void *buf, size_t len) {
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

    char portbuf[16];
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
    return -1;
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

    return reply_msg;

    
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
