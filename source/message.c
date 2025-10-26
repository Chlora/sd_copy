/**
 * @file message.c
 * 
 * @brief Common network I/O functions for client and server
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include "message-private.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int read_all(int socket, void *buffer, size_t n) {
    size_t bytes_read = 0;
    ssize_t result;
    char *buf = (char *)buffer;
    
    while (bytes_read < n) {
        result = read(socket, buf + bytes_read, n - bytes_read);
        
        if (result < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        
        if (result == 0) {
            return (bytes_read > 0) ? bytes_read : 0;
        }
        
        bytes_read += result;
    }
    
    return bytes_read;
}

int write_all(int socket, const void *buffer, size_t n) {
    size_t bytes_written = 0;
    ssize_t result;
    const char *buf = (const char *)buffer;
    
    while (bytes_written < n) {
        result = write(socket, buf + bytes_written, n - bytes_written);
        
        if (result < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        
        bytes_written += result;
    }
    
    return bytes_written;
}

void debug_message(const char *prefix, MessageT *msg) {
    if (msg == NULL) {
        printf("[DEBUG] %s: NULL message\n", prefix);
        return;
    }
    
    printf("\n========================================\n");
    printf("[DEBUG] %s\n", prefix);
    printf("========================================\n");
    
    // Print opcode
    printf("Opcode: %d ", msg->opcode);
    switch (msg->opcode) {
        case MESSAGE_T__OPCODE__OP_BAD:           printf("(OP_BAD)\n"); break;
        case MESSAGE_T__OPCODE__OP_ADD:           printf("(OP_ADD)\n"); break;
        case MESSAGE_T__OPCODE__OP_GET:           printf("(OP_GET)\n"); break;
        case MESSAGE_T__OPCODE__OP_DEL:           printf("(OP_DEL)\n"); break;
        case MESSAGE_T__OPCODE__OP_SIZE:          printf("(OP_SIZE)\n"); break;
        case MESSAGE_T__OPCODE__OP_GETMODELS:     printf("(OP_GETMODELS)\n"); break;
        case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR: printf("(OP_GETLISTBYTEAR)\n"); break;
        case MESSAGE_T__OPCODE__OP_ORDER:         printf("(OP_ORDER)\n"); break;
        case MESSAGE_T__OPCODE__OP_ERROR:         printf("(OP_ERROR)\n"); break;
        default:                                   printf("(UNKNOWN: %d)\n", msg->opcode); break;
    }
    
    // Print c_type
    printf("C_Type: %d ", msg->c_type);
    switch (msg->c_type) {
        case MESSAGE_T__C_TYPE__CT_BAD:    printf("(CT_BAD)\n"); break;
        case MESSAGE_T__C_TYPE__CT_DATA:   printf("(CT_DATA)\n"); break;
        case MESSAGE_T__C_TYPE__CT_MARCA:  printf("(CT_MARCA)\n"); break;
        case MESSAGE_T__C_TYPE__CT_YEAR:   printf("(CT_YEAR)\n"); break;
        case MESSAGE_T__C_TYPE__CT_MODEL:  printf("(CT_MODEL)\n"); break;
        case MESSAGE_T__C_TYPE__CT_RESULT: printf("(CT_RESULT)\n"); break;
        case MESSAGE_T__C_TYPE__CT_LIST:   printf("(CT_LIST)\n"); break;
        case MESSAGE_T__C_TYPE__CT_NONE:   printf("(CT_NONE)\n"); break;
        default:                            printf("(UNKNOWN: %d)\n", msg->c_type); break;
    }
    
    // Print data field
    if (msg->data != NULL) {
        printf("Data:\n");
        printf("  ano: %d\n", msg->data->ano);
        printf("  preco: %.2f\n", msg->data->preco);
        printf("  marca: %d ", msg->data->marca);
        switch (msg->data->marca) {
            case MARCA__MARCA_TOYOTA:   printf("(TOYOTA)\n"); break;
            case MARCA__MARCA_BMW:      printf("(BMW)\n"); break;
            case MARCA__MARCA_RENAULT:  printf("(RENAULT)\n"); break;
            case MARCA__MARCA_AUDI:     printf("(AUDI)\n"); break;
            case MARCA__MARCA_MERCEDES: printf("(MERCEDES)\n"); break;
            default:                     printf("(UNKNOWN)\n"); break;
        }
        printf("  modelo: %s\n", msg->data->modelo ? msg->data->modelo : "(null)");
        printf("  combustivel: %d ", msg->data->combustivel);
        switch (msg->data->combustivel) {
            case COMBUSTIVEL__COMBUSTIVEL_GASOLINA: printf("(GASOLINA)\n"); break;
            case COMBUSTIVEL__COMBUSTIVEL_GASOLEO:  printf("(GASOLEO)\n"); break;
            case COMBUSTIVEL__COMBUSTIVEL_ELETRICO: printf("(ELETRICO)\n"); break;
            case COMBUSTIVEL__COMBUSTIVEL_HIBRIDO:  printf("(HIBRIDO)\n"); break;
            default:                                 printf("(UNKNOWN)\n"); break;
        }
    } else {
        printf("Data: NULL\n");
    }
    
    // Print result field
    printf("Result: %d\n", msg->result);
    
    // Print models array
    if (msg->n_models > 0 && msg->models != NULL) {
        printf("Models (%zu):\n", msg->n_models);
        for (size_t i = 0; i < msg->n_models; i++) {
            printf("  [%zu]: %s\n", i, msg->models[i] ? msg->models[i] : "(null)");
        }
    } else {
        printf("Models: (empty)\n");
    }
    
    // Print cars array
    if (msg->n_cars > 0 && msg->cars != NULL) {
        printf("Cars (%zu):\n", msg->n_cars);
        for (size_t i = 0; i < msg->n_cars && i < 5; i++) { // Limit to first 5
            if (msg->cars[i] != NULL) {
                printf("  [%zu]: %d %s %.2f\n", i, 
                       msg->cars[i]->ano, 
                       msg->cars[i]->modelo ? msg->cars[i]->modelo : "(null)",
                       msg->cars[i]->preco);
            } else {
                printf("  [%zu]: NULL\n", i);
            }
        }
        if (msg->n_cars > 5) {
            printf("  ... (%zu more)\n", msg->n_cars - 5);
        }
    } else {
        printf("Cars: (empty)\n");
    }
    
    printf("========================================\n\n");
}

struct data_t *pb_to_data(Data *pb) {
    if (pb == NULL || pb->modelo == NULL) return NULL;
    
    return data_create( pb->ano, 
                        pb->preco, 
                        (enum marca_t)pb->marca,
                        pb->modelo, 
                        (enum combustivel_t)pb->combustivel);
}

Data *data_to_pb(struct data_t *data) {
    if (data == NULL || data->modelo == NULL) return NULL;
    
    Data *pb = malloc(sizeof(Data));
    if (pb == NULL) return NULL;
    
    data__init(pb);
    pb->ano = data->ano;
    pb->preco = data->preco;
    pb->marca = (Marca)data->marca;
    pb->combustivel = (Combustivel)data->combustivel;
    pb->modelo = strdup(data->modelo);
    
    if (pb->modelo == NULL) {
        free(pb);
        return NULL;
    }
    
    return pb;
}

void free_pb_data_array(Data **pb_array, size_t count) {
    if (pb_array == NULL) return;
    for (size_t i = 0; i < count; i++) {
        data__free_unpacked(pb_array[i], NULL);
    }
    free(pb_array);
}

Data **data_array_to_pb(struct data_t **arr, size_t *size) {
    if (arr == NULL || size == NULL) return NULL;
    
    // Contar elementos
    size_t n = 0;
    while (arr[n] != NULL) n++;
    
    *size = n;
    if (n == 0) return NULL;
    
    // Alocar e converter
    Data **pb_arr = malloc(sizeof(Data*) * n);
    if (pb_arr == NULL) return NULL;
    
    for (size_t i = 0; i < n; i++) {
        pb_arr[i] = data_to_pb(arr[i]);
        if (pb_arr[i] == NULL) {
            free_pb_data_array(pb_arr, i);
            return NULL;
        }
    }
    
    return pb_arr;
}

struct data_t **pb_to_data_array(Data **pb_arr, size_t count) {
    if (pb_arr == NULL) return NULL;
    
    if (count == 0) return NULL;
    
    // Allocate array with count + 1 elements (last will be NULL)
    struct data_t **arr = malloc(sizeof(struct data_t*) * (count + 1));
    if (arr == NULL) return NULL;
    
    // Convert each element
    for (size_t i = 0; i < count; i++) {
        arr[i] = pb_to_data(pb_arr[i]);
        if (arr[i] == NULL) {
            // Free already converted elements on error
            for (size_t j = 0; j < i; j++) {
                data_destroy(arr[j]);
            }
            free(arr);
            return NULL;
        }
    }
    
    // NULL-terminate the array
    arr[count] = NULL;
    
    return arr;
}