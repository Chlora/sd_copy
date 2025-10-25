#include <errno.h>
#include "../include/network_client.h"
#include <arpa/inet.h>


int write_all(int sock, void *buf, int len) {
    int total = 0;
    char *p = (char *)buf;

    while (total < len) {
        int n = send(sock, p + total, len - total, 0);
        if (n < 0) {
            
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        total += n;
    }
    return total;
}

int read_all(int sock, void *buf, int len) {
    int total = 0;
    char *p = (char *)buf;

    while (total < len) {
        int n = recv(sock, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        total += n;
    }
    return total;
}