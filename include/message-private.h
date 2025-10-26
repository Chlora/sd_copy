/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 4/10/2025
 */
#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

#include "sdmessage.pb-c.h"
#include "data.h"
#include <stdlib.h>

int write_all(int socket, const void *buffer, size_t n);

int read_all(int socket, void *buffer, size_t n);

void debug_message(const char *prefix, MessageT *msg);

struct data_t *pb_to_data(Data *pb);

Data *data_to_pb(struct data_t *data);

void free_pb_data_array(Data **pb_array, size_t count);

Data **data_array_to_pb(struct data_t **arr, size_t *size);

struct data_t **pb_to_data_array(Data **pb_arr, size_t count);

#endif