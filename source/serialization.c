#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "serialization.h"
#include "list-private.h"

int car_to_buffer(struct data_t *car, char **car_buf) {
    if (!car || !car_buf || !car->modelo) return -1;

    size_t modelo_len = strlen(car->modelo) + 1;
    size_t buf_sz = 5 * sizeof(uint32_t) + modelo_len;

    uint8_t *buf = malloc(buf_sz);
    if (!buf) return -1;

    uint8_t *ptr = buf;
    uint32_t u32;

    u32 = htonl((uint32_t)car->ano);
    memcpy(ptr, &u32, sizeof u32); ptr += sizeof u32;

    if (sizeof(float) != 4) { free(buf); return -1; }
    uint32_t preco_u32;
    memcpy(&preco_u32, &car->preco, sizeof preco_u32);
    preco_u32 = htonl(preco_u32);
    memcpy(ptr, &preco_u32, sizeof preco_u32); ptr += sizeof preco_u32;

    u32 = htonl((uint32_t)car->marca);
    memcpy(ptr, &u32, sizeof u32); ptr += sizeof u32;

    u32 = htonl((uint32_t)car->combustivel);
    memcpy(ptr, &u32, sizeof u32); ptr += sizeof u32;

    u32 = htonl((uint32_t)modelo_len);
    memcpy(ptr, &u32, sizeof u32); ptr += sizeof u32;

    memcpy(ptr, car->modelo, modelo_len);

    *car_buf = (char *)buf;
    return (int)buf_sz;
}

struct data_t *buffer_to_car(char *car_buf) {
    if (!car_buf) return NULL;

    const uint8_t *ptr = (const uint8_t *)car_buf;
    uint32_t u32;

    memcpy(&u32, ptr, sizeof u32); ptr += sizeof u32;
    int ano = (int)ntohl(u32);

    if (sizeof(float) != 4) return NULL;
    memcpy(&u32, ptr, sizeof u32); ptr += sizeof u32;
    u32 = ntohl(u32);
    float preco;
    memcpy(&preco, &u32, sizeof preco);

    memcpy(&u32, ptr, sizeof u32); ptr += sizeof u32;
    enum marca_t marca = (enum marca_t)ntohl(u32);

    memcpy(&u32, ptr, sizeof u32); ptr += sizeof u32;
    enum combustivel_t comb = (enum combustivel_t)ntohl(u32);

    memcpy(&u32, ptr, sizeof u32); ptr += sizeof u32;
    uint32_t modelo_len = ntohl(u32);
    if (modelo_len == 0) return NULL;

    char *modelo = malloc(modelo_len);
    if (!modelo) return NULL;
    memcpy(modelo, ptr, modelo_len);

    struct data_t *car = data_create(ano, preco, marca, modelo, comb);
    free(modelo);

    return car;
}


int car_list_to_buffer(struct list_t *list, char **list_buf) {
    if (!list || !list_buf) return -1;

    uint32_t count = 0;
    size_t total = sizeof(uint32_t);
    for (struct car_t *cur = list->head; cur; cur = cur->next) {
        if (!cur->data || !cur->data->modelo) continue;
        count++;
        total += 5 * sizeof(uint32_t) + strlen(cur->data->modelo) + 1;
    }

    uint8_t *buf = malloc(total);
    if (!buf) return -1;

    uint8_t *p = buf;
    uint32_t u32 = htonl(count);
    memcpy(p, &u32, sizeof u32); p += sizeof u32;

    for (struct car_t *cur = list->head; cur; cur = cur->next) {
        if (!cur->data || !cur->data->modelo) continue;
        char *car_part = NULL;
        int part_len = car_to_buffer(cur->data, &car_part);
        if (part_len < 0 || !car_part) { free(buf); return -1; }
        memcpy(p, car_part, (size_t)part_len);
        p += part_len;
        free(car_part);
    }

    *list_buf = (char *)buf;
    return (int)total;
}

struct list_t *buffer_to_car_list(char *list_buf) {
    if (!list_buf) return NULL;

    const uint8_t *p = (const uint8_t *)list_buf;
    uint32_t u32; memcpy(&u32, p, 4); p += 4;
    uint32_t count = ntohl(u32);

    struct list_t *list = list_create();
    if (!list) return NULL;

    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *start = p;

        memcpy(&u32, start + 16, 4);
        uint32_t modelo_len = ntohl(u32);

        struct data_t *car = buffer_to_car((char *)start);
        if (!car || list_add(list, car) != 0) { list_destroy(list); return NULL; }

        p = start + 5 * 4 + modelo_len;
    }
    return list;
}
