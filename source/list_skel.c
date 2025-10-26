/**
 * @file list_skel.c
 * 
 * @brief Server-side skeleton implementation for list operations
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include "list_skel.h"
#include "message-private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list_t *list_skel_init() {
    return list_create();
}

int list_skel_destroy(struct list_t *list) {
    return (list == NULL) ? -1 : list_destroy(list);
}

static void set_error(MessageT *msg) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
}

static void set_success(MessageT *msg, MessageT__CType type) {
    msg->opcode++;
    msg->c_type = type;
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

static void handle_getlistbyear(MessageT *msg, struct list_t *list) {
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
    if (msg->c_type != MESSAGE_T__C_TYPE__CT_MODEL || msg->models == NULL || 
        msg->n_models == 0 || msg->models[0] == NULL) {
        set_error(msg);
        return;
    }

    int result = list_remove_by_model(list, msg->models[0]);
    if (result != 0) {
        set_error(msg);
        return;
    }

    msg->result = result;
    set_success(msg, MESSAGE_T__C_TYPE__CT_RESULT);
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
            handle_getlistbyear(msg, list);
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