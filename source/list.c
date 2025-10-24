#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/data.h"
#include "../include/list.h"
#include "../include/list-private.h"

struct list_t; /* Definida em list-private.h */

/* Cria e inicializa uma nova lista de carros.
 * Retorna a lista ou NULL em caso de erro.
 */
struct list_t *list_create() {
    struct list_t *list = malloc(sizeof(struct list_t));
    if (!list) return NULL;

    list->head = NULL;
    list->size = 0;

    return list;
}

/* Elimina a lista, libertando toda a memória ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_destroy(struct list_t *list) {
    if (!list) return -1;

    struct car_t *current = list->head;
    while (current) {
        struct car_t *next = current->next;

        if (current->data) {
            data_destroy(current->data);
        }
        free(current);

        current = next;
    }

    free(list);
    return 0;
}

/* Adiciona um novo carro à lista.
 * O carro é inserido na última posição da lista.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_add(struct list_t *list, struct data_t *car) {
    if (!list || !car) return -1;

    struct car_t *node = malloc(sizeof *node);
    if (!node) return -1;

    node->data = car;
    node->next = NULL;

    if (!list->head) {
        list->head = node;
    } else {
        struct car_t *cur = list->head;
        while (cur->next) cur = cur->next;
        cur->next = node;
    }

    list->size++;
    return 0;
}

/* Remove da lista o primeiro carro que corresponda ao modelo indicado.
 * Retorna 0 se encontrou e removeu, 1 se não encontrou, ou -1 em caso de erro.
 */
int list_remove_by_model(struct list_t *list, const char *modelo) {
    if (!list || !modelo) return -1;
    if (!list->head) return 1;

    struct car_t *cur = list->head, *prev = NULL;

    while (cur) {
        struct data_t *car = cur->data;
        if (car && car->modelo && strcmp(car->modelo, modelo) == 0) {
            if (prev) prev->next = cur->next;
            else      list->head = cur->next;

            data_destroy(car);
            free(cur); 
            list->size--; 
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    return 1; 
}

/* Obtém o primeiro carro que corresponda à marca indicada.
 * Retorna ponteiro para os dados ou NULL se não encontrar ou em caso de erro.
 */
struct data_t *list_get_by_marca(struct list_t *list, enum marca_t marca) {
    if (!list) return NULL;

    for (struct car_t *cur = list->head; cur; cur = cur->next) {
        if (cur->data && cur->data->marca == marca) {
            return cur->data;
        }
    }
    return NULL;
}

/* Obtém um array de ponteiros para carros de um determinado ano.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
struct data_t **list_get_by_year(struct list_t *list, int ano) {
    if (!list) return NULL;

    size_t count = 0;
    for (struct car_t *cur = list->head; cur; cur = cur->next) {
        if (cur->data && cur->data->ano == ano) {
            count++;
        }  
    }
    
    struct data_t **arr = malloc((count + 1) * sizeof *arr);
    if (!arr) return NULL;

    size_t i = 0;
    for (struct car_t *cur = list->head; cur; cur = cur->next) {
        if (cur->data && cur->data->ano == ano) {
            arr[i++] = cur->data;
        }
    }
        
    arr[i] = NULL;
    return arr;
}

/* Ordena a lista de carros por ano de fabrico (crescente).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_order_by_year(struct list_t *list) {
    if (!list) return -1;
    if (!list->head || !list->head->next) return 0;

    int swapped;
    do {
        swapped = 0;
        for (struct car_t *p = list->head; p->next; p = p->next) {
            if (!p->data || !p->next->data) return -1;
            if (p->data->ano > p->next->data->ano) {
                struct data_t *tmp = p->data;
                p->data = p->next->data;
                p->next->data = tmp;
                swapped = 1;
            }
        }
    } while (swapped);

    return 0;
}

/* Retorna o número de carros na lista ou -1 em caso de erro.
 */
int list_size(struct list_t *list) {
    if (!list) {return -1;}

    return list->size;
}

/* Constrói um array de strings com os modelos dos carros na lista.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
char **list_get_model_list(struct list_t *list) {
    if (!list) return NULL;

    size_t cap = (size_t) (list->size >= 0 ? list->size : 0);
    char **models = malloc((cap + 1) * sizeof *models);
    if (!models) return NULL;

    size_t i = 0;
    for (struct car_t *cur = list->head; cur; cur = cur->next) {
        if (!cur->data || !cur->data->modelo) continue;
        char *dup = strdup(cur->data->modelo);
        if (!dup) {
            while (i > 0) free(models[--i]);
            free(models);
            return NULL;
        }
        models[i++] = dup;
    }
    models[i] = NULL;

    char **tmp = realloc(models, (i + 1) * sizeof *tmp);
    return tmp ? tmp : models;
}

/* Liberta a memória ocupada pelo array de modelos.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_free_model_list(char **models) {
    if (!models) return -1;
    for (size_t i = 0; models[i]; ++i) free(models[i]);
    free(models);
    return 0;
}

struct data_t **list_get_all(struct list_t *list) {
    if (list == NULL) {
        return NULL;
    }
    
    // Alocar array com size + 1 elementos (o último será NULL)
    struct data_t **array = malloc(sizeof(struct data_t*) * (list->size + 1));
    if (array == NULL) {
        return NULL;
    }
    
    // Percorrer a lista e preencher o array com ponteiros para os dados
    struct car_t *current = list->head;
    int i = 0;
    
    while (current != NULL) {
        // IMPORTANTE: Aponta diretamente para os dados internos (não duplica)
        array[i] = current->data;
        current = current->next;
        i++;
    }
    
    // Terminar o array com NULL
    array[i] = NULL;
    
    return array;
}