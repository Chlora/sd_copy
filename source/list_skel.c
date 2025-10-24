/**
 * @file list_skel.h
 * 
 * @brief
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include "list_skel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list_t *list_skel_init() {
    return list_create();
}

int list_skel_destroy(struct list_t *list) {
    return (list == NULL) ? -1 : list_destroy(list);
}

static void debug_print_message(const char *prefix, MessageT *msg) {
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

static void set_error(MessageT *msg) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
}

static void set_success(MessageT *msg, MessageT__CType type) {
    msg->opcode++;
    msg->c_type = type;
}

static struct data_t *pb_to_data(Data *pb) {
    if (pb == NULL || pb->modelo == NULL) return NULL;
    
    return data_create( pb->ano, 
                        pb->preco, 
                        (enum marca_t)pb->marca,
                        pb->modelo, 
                        (enum combustivel_t)pb->combustivel);
}

static Data *data_to_pb(struct data_t *data) {
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

static void free_pb_data_array(Data **pb_array, size_t count) {
    if (pb_array == NULL) return;
    for (size_t i = 0; i < count; i++) {
        data__free_unpacked(pb_array[i], NULL);
    }
    free(pb_array);
}

static Data **data_array_to_pb(struct data_t **arr, size_t *size) {
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

static void handle_add(MessageT *msg, struct list_t *list) {
    if (msg->c_type != MESSAGE_T__C_TYPE__CT_DATA || msg->data == NULL) {
        set_error(msg);
        return;
    }

    struct data_t *data = pb_to_data(msg->data);
    if (data == NULL || list_add(list, data) == -1) {
        set_error(msg);
        if (data != NULL) data_destroy(data);
        return;
    }

    set_success(msg, MESSAGE_T__C_TYPE__CT_NONE);
}

static void handle_get(MessageT *msg, struct list_t *list) {
    if (msg->data == NULL) {
        set_error(msg);
        return;
    }
    
    switch (msg->c_type)
    {
    case MESSAGE_T__C_TYPE__CT_MARCA:
        struct data_t *car = list_get_by_marca(list, (enum marca_t)msg->data->marca);
        if (car == NULL) {
            set_error(msg);
            return;
        }

        Data *pb_data = data_to_pb(car);
        if (pb_data == NULL) {
            set_error(msg);
            return;
        }

        // set responde data
        data__free_unpacked(msg->data, NULL);
        msg->data = pb_data;
        set_success(msg, MESSAGE_T__C_TYPE__CT_DATA);
        break;
    
    case MESSAGE_T__C_TYPE__CT_YEAR:
        struct data_t **cars = list_get_by_year(list, (int)msg->data->ano);
        if (cars == NULL) {
            set_error(msg);
            return;
        }

        size_t size = 0;
        Data **pb_cars = data_array_to_pb(cars, &size);
        free(cars);

        if (pb_cars == NULL  && size > 0) {
            set_error(msg);
            return;
        }

        free_pb_data_array(msg->cars, msg->n_cars);
        msg->cars = pb_cars;
        msg->n_cars = size;
        set_success(msg, MESSAGE_T__C_TYPE__CT_YEAR);
        break;
    default:
        set_error(msg);
        return;
    }
}

static void handle_getmodels(MessageT *msg, struct list_t *list) {
    if (msg->c_type != MESSAGE_T__C_TYPE__CT_NONE) {
        set_error(msg);
        return;
    }

    char **models = list_get_model_list(list);
    if (models == NULL) {
        set_error(msg);
        return;
    }

    size_t size = 0;
    while (models[size] != NULL) size++;

    list_free_model_list(msg->models);

    msg->n_models = size;
    msg->models = models;
    set_success(msg, MESSAGE_T__C_TYPE__CT_MODEL);
}

static void handle_getlistbytear(MessageT *msg, struct list_t *list) {
    if (msg->c_type != MESSAGE_T__C_TYPE__CT_NONE) {
        set_error(msg);
        return;
    }

    if (list_order_by_year(list) == -1) {
        set_error(msg);
        return;
    }

    struct data_t **sorted = list_get_all(list);
    if (sorted == NULL) {
        set_error(msg);
        return;
    }

    size_t size = 0;
    Data **pb_sorted = data_array_to_pb(sorted, &size);
    free(sorted);

    if (pb_sorted == NULL  && size > 0) {
        set_error(msg);
        return;
    }

    free_pb_data_array(msg->cars, msg->n_cars);
    msg->cars = pb_sorted;
    msg->n_cars = size;
    set_success(msg, MESSAGE_T__C_TYPE__CT_LIST);
}

static void handle_del(MessageT *msg, struct list_t *list) {
    if (msg->c_type != MESSAGE_T__C_TYPE__CT_MODEL || msg->models == NULL) {
        set_error(msg);
        return;
    }

    if (list_remove_by_model(list, msg->models[0]) == -1) {
        set_error(msg);
        return;
    }

    set_success(msg, MESSAGE_T__C_TYPE__CT_NONE);
}

static void handle_size(MessageT *msg, struct list_t *list) {
    if (msg->c_type != MESSAGE_T__C_TYPE__CT_NONE) {
        set_error(msg);
        return;
    }

    int size = list_size(list);
    if (size >= 0) {
        msg->result = size;
        set_success(msg, MESSAGE_T__C_TYPE__CT_RESULT);
    } else {
        set_error(msg);
    }
}

int invoke(MessageT *msg, struct list_t *list) {
    if (msg == NULL || list == NULL) {
        return -1;
    }

    // Debug print
    debug_print_message("INCOMING REQUEST", msg);

    switch (msg->opcode) {
        case MESSAGE_T__OPCODE__OP_ADD:
            handle_add(msg, list);
            break;

        case MESSAGE_T__OPCODE__OP_GET:
            handle_get(msg, list);
            break;

        case MESSAGE_T__OPCODE__OP_GETMODELS:
            handle_getmodels(msg, list);
            break;

        case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR:
            handle_getlistbytear(msg, list);
            break;

        case MESSAGE_T__OPCODE__OP_DEL:
            handle_del(msg, list);
            break;

        case MESSAGE_T__OPCODE__OP_SIZE:
            handle_size(msg, list);
            break;
            
        default:
            set_error(msg);
            break; // Erro: operação desconhecida
    }

    return 0; // Sucesso
}