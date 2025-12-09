/**
 * @file zookeeper-interactions.c
 * 
 * @brief Client-side module for interacting with Zookeper
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#define THREADED

#include "client_stub-private.h"
#include "client_stub.h"
#include <zookeeper/zookeeper.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

sem_t mutex;

struct rlist_t;

static zhandle_t *zkHandler = NULL;
struct rlist_t *rlistHead = NULL;
struct rlist_t *rlistTail = NULL;

void connectToHead(struct String_vector children) {
    sem_wait(&mutex);

    const char *child = children.data[0];

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", "/chain", child);

    int buflen = 512;
    char *childip = malloc(buflen + 1);
    if (!childip) {
        printf("Erro ao alocar memória para o buffer.\n");
        sem_post(&mutex);
        return;
    }

    int rc = zoo_get(zkHandler, path, 0, childip, &buflen, NULL);
    // zoo ok
    if (rc == ZOK) {
        childip[buflen] = '\0';
        rlistHead = rlist_connect(childip);
    } else {
        printf("Erro ao obter dados do Zookeeper sobre o servidor head.\n");
    }

    sem_post(&mutex);

    free(childip);
}

void connectToTail(struct String_vector children) {
    sem_wait(&mutex);

    const char *child = children.data[children.count - 1];

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", "/chain", child);

    int buflen = 512;
    char *childip = malloc(buflen + 1);
    if (!childip) {
        printf("Erro ao alocar memória para o buffer.\n");
        sem_post(&mutex);
        return;
    }

    int rc = zoo_get(zkHandler, path, 0, childip, &buflen, NULL);
    // zoo ok
    if (rc == ZOK) {
        childip[buflen] = '\0';
        rlistTail = rlist_connect(childip);
    } else {
        printf("Erro ao obter dados do Zookeeper sobre o servidor tail.\n");
    }

    sem_post(&mutex);

    free(childip); 
}


void watcher(zhandle_t *zkH, int type, int state, const char *path, void *watcherCtx) {
    (void)zkH;
    (void)watcherCtx;
    (void)path;

    if (!zkHandler) {
        return;
    }

    if (type == ZOO_CHILD_EVENT) {

        struct String_vector children;
        int rc = zoo_wget_children(zkHandler, "/chain", watcher, NULL, &children);
        if (rc != ZOK) {
            printf("Erro ao fazer zoo_wget_children.\n");
            return;
        }

        if (children.count == 0) {
            if (rlistHead) {
                rlist_disconnect(rlistHead);
                rlistHead = NULL;
            }
            if (rlistTail) {
                rlist_disconnect(rlistTail);
                rlistTail = NULL;
            }
            deallocate_String_vector(&children);
            printf("Nenhum servidor disponível em /chain.\n");
            return;
        }

        sem_wait(&mutex);

        //desconecta primeiro as ligacoes antigas (se existirem)
        // connectToHead/connectToTail vao criar novas ligacoes
        if (rlistHead) {
            rlist_disconnect(rlistHead);
            rlistHead = NULL;
        }
        if (rlistTail) {
            rlist_disconnect(rlistTail);
            rlistTail = NULL;
        }

        sem_post(&mutex);

        // Dar tempo ao servidor para abrir a socket
        sleep(1);

        // tenta ligar ao novo head e tail
        connectToHead(children);
        connectToTail(children);

        deallocate_String_vector(&children); 
        printf("Servidores head/tail atualizados pelo watcher.\n");
        return;
    }

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            // sessao estabelecida: nao faz nada
            printf("Sessão ZK establecida.\n");
            return;
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            //sessao expirada: fecha ligacao local ao ZooKeeper e limpa conexoes 
            printf("Sessão ZK expirou, a fechar a ligação...\n");
            if (zkHandler) {
                zookeeper_close(zkHandler);
                zkHandler = NULL;
            }
            if (rlistHead) {
                rlist_disconnect(rlistHead);
                rlistHead = NULL;
            }
            if (rlistTail) {
                rlist_disconnect(rlistTail);
                rlistTail = NULL;
            }
            return;
        }
    }
}



int disconnectFromZookeeper() {
    sem_wait(&mutex);

    int result1 = zookeeper_close(zkHandler);

    sem_post(&mutex);
    int result4 = sem_destroy(&mutex);

    if (result4 == -1) {
        printf("Erro ao tentar destruir o semaforo.\n");
    }

    return (result1 == ZOK) ? 0 : -1;
}


int connectToZookeper(char* ip) {
    sem_init(&mutex, 0, 1);
    zkHandler = zookeeper_init(ip, watcher, 10000, 0, 0, 0);

    if (!zkHandler) {
        printf("Erro ao establecer ligação com o Zookeeper.%s\n", ip);
        return -1;
    }
    struct String_vector children;
    int rc = zoo_get_children(zkHandler, "/chain", 1, &children);

    if (children.count <= 0 || children.data == NULL) {
        printf("Não existem servidores.\n");
        deallocate_String_vector(&children);
        return -1; 
    }

    // zoo ok
    if (rc != ZOK) {
        printf("Erro ao obter dados do Zookeeper sobre os servidores.\n");
        int result = deallocate_String_vector(&children);
        if (result == -1) {
            printf("Erro ao dealocar o string vector.\n");
        }
        return -1;
    }

    connectToHead(children);
    connectToTail(children);

    if (!rlistHead || !rlistTail) {
        return -1;
    }

    int result = deallocate_String_vector(&children);
    if (result == -1) {
        printf("Erro ao dealocar o string vector.\n");
    }
    printf("Conexão establecida com o Zookeeper.\n");

    return 0;
}


int areConnectionsEstablished() {
    int s;
    int result = sem_getvalue(&mutex, &s);

    if (result == -1) {
        printf("Erro ao tentar aceder ao valor do semaforo.\n");
        return -1;
    }

    return (s == 1 && rlistHead != NULL && rlistTail != NULL) ? 1 : 0;
}



