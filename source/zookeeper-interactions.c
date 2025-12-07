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

#include "client_stub-private.h"
#include "client_stub.h"
#include <zookeeper/zookeeper.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct rlist_t;

static zhandle_t *zkHandler = NULL;
struct rlist_t *rlistHead = NULL;
struct rlist_t *rlistTail = NULL;

void connectToHead(struct String_vector children) {
    const char *child = children.data[0];

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", "/chain", child);

    int buflen = 512;
    char *childip = malloc(buflen + 1);
    if (!childip) {
        printf("Erro ao alocar memória para o buffer.\n");
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

    free(childip);    
}

void connectToTail(struct String_vector children) {
    const char *child = children.data[children.count - 1];

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", "/chain", child);

    int buflen = 512;
    char *childip = malloc(buflen + 1);
    if (!childip) {
        printf("Erro ao alocar memória para o buffer.\n");
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

    free(childip); 
}


void watcher(zhandle_t *zkH, int type, int state, const char *path, void *watcherCtx) {

    if (type == ZOO_CHILD_EVENT) {

    }

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            // nada?
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            zookeeper_close(zkH);
        }
    }
}


int disconnectFromZookeeper() {
    int result1 = zookeeper_close(zkHandler);
    int result2 = rlist_disconnect(rlistHead);
    int result3 = rlist_disconnect(rlistTail);

    if (result1 != ZOK) {
        printf("Erro ao tentar desligar do Zookeeper.\n");
    }
    if (result2 == -1) {
        printf("Erro ao tentar desligar do servidor head.\n");
    }
    if (result3 == -1) {
        printf("Erro ao tentar desligar do servidor tail.\n");
    }

    return (result1 + result2 + result3 == 0) ? 0 : -1;
}


int connectToZookeper(char* ip) {
    zkHandler = zookeeper_init(ip, watcher, 10000, 0, 0, 0);

    if (!zkHandler) {
        printf("Erro ao establecer ligação com o Zookeeper.%s\n", ip);
        return -1;
    }
    struct String_vector children;
    int rc = zoo_get_children(zkHandler, "/chain", 1, &children);

    // zoo ok
    if (rc != ZOK) {
        printf("Erro ao obter dados do Zookeeper sobre os servidores.\n");
        int result = deallocate_String_vector(&children);
        if (result == -1) {
            printf("Erro ao dealocar o string vector.\n");
        }
        disconnectFromZookeeper();
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
    if (rlistHead != NULL && rlistTail != NULL) {
        return 1;
    }
    return 0;
}



