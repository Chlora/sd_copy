/**
 * @file data.c
 * 
 * @brief Implementation of data structure functions
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include <stdio.h>
#include "data.h"
#include <stdlib.h>
#include <string.h>

/* Função que cria um novo elemento de dados data_t.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct data_t *data_create(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel) {
    if (!modelo) return NULL;

    struct data_t *data = malloc(sizeof(struct data_t));
    if (!data) return NULL;

    data->modelo = malloc(strlen(modelo) + 1); 
    if (!data->modelo) {free(data); return NULL;}
    strcpy(data->modelo, modelo);

    data->ano = ano;
    data->preco = preco;
    data->marca = marca;
    data->combustivel = combustivel;

    return data;
}

/* Função que elimina um bloco de dados, libertando toda a memória por ele ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int data_destroy(struct data_t *data) {
    if (!data) {return -1;}

    free(data->modelo);
    free(data);

    return 0;
}

/* Função que duplica uma estrutura data_t.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct data_t *data_dup(struct data_t *data) {
    if (!data || !data->modelo) {return NULL;}

    struct data_t *newdata = malloc(sizeof(struct data_t));
    
    newdata->modelo = malloc(strlen(data->modelo) + 1); 
    if (!data->modelo) {free(newdata); return NULL;}
    strcpy(newdata->modelo, data->modelo);

    newdata->ano = data->ano;
    newdata->preco = data->preco;
    newdata->marca = data->marca;
    newdata->combustivel = data->combustivel;

    return newdata;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int data_replace(struct data_t *data, int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel) {
    if (!data || !modelo) {return -1;}

    char *new_modelo = malloc(strlen(modelo) + 1);
    if (!new_modelo) return -1;
    strcpy(new_modelo, modelo);

    free(data->modelo);
    data->modelo = new_modelo;

    data->ano = ano;
    data->preco = preco;
    data->marca = marca;
    data->combustivel = combustivel;

    return 0;
}