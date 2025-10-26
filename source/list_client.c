#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/client_stub.h"
#include "../include/client_stub-private.h"
#include "../include/network_client.h"

#include "../include/data.h"
#include "../include/list.h"
#include "../include/list-private.h"

struct rlist_t *rlist = NULL;

#define MAX_ARGS 2
int quitSignal = 0;


int commandAdd(char *data) {

}


int commandGetByMarca(char *marca) {
    enum marca_t marca_enum;
    if (strcmp(marca, "TOYOTA") == 0) {
        marca_enum = MARCA_TOYOTA;
    } else if (strcmp(marca, "BMW") == 0) {
        marca_enum = MARCA_BMW;
    } else if (strcmp(marca, "AUDI") == 0) {
        marca_enum = MARCA_AUDI;
    } else if (strcmp(marca, "RENAULT") == 0) {
        marca_enum = MARCA_RENAULT;
    } else if (strcmp(marca, "MERCEDES") == 0) {
        marca_enum = MARCA_MERCEDES;
    } else {
        printf("ERRO : Marca desconhecida '%s'.\n", marca);
        printf("Marcas válidas: TOYOTA, BMW, AUDI, RENAULT, MERCEDES.\n");
        return -1;
    }

    struct data_t *result = rlist_get_by_marca(rlist, marca_enum);
    if (result == NULL) {
        printf("ERRO : Falha ao obter carro da marca %s.\n", marca);
        return -1;
    }
    printf("Carro da marca %s:\n", marca);
    printf("Modelo: %s, Marca: %d, Ano: %d\n", result->modelo, result->marca, result->ano);
    data_destroy(result);
    
    return 0;
}


int commandGetByYear(char *year) {
    int year_int = atoi(year);
    struct data_t **result = rlist_get_by_year(rlist, year_int);

    if (result == NULL) {
        printf("ERRO : Falha ao obter carros do ano %d.\n", year_int);
        return -1;
    }
    if (result[0] == NULL) {
        printf("Nenhum carro encontrado do ano %d.\n", year_int);
        free(result);
        return 0;
    }
    printf("Carros do ano %d:\n", year_int);
    for (int i = 0; result[i] != NULL; i++) {
        struct data_t *car = result[i];
        printf("Modelo: %s, Marca: %d, Ano: %d\n", car->modelo, car->marca, car->ano);
        data_destroy(car);
    }
    free(result);

    return 0;
}


int commandGetModelList() {
    char **arr = rlist_get_model_list(rlist);
    if (arr == NULL) {
        printf("ERRO : Falha ao obter a lista de modelos.\n");
        return -1;
    }
    printf("Lista de modelos:\n");
    for (int i = 0; arr[i] != NULL; i++) {
        printf("%s\n", arr[i]);
        free(arr[i]);
    }
    free(arr);

    return 0;
}


int commandGetListOrderedByYear() {
    int result = rlist_order_by_year(rlist);
    if (result == -1) {
        printf("ERRO : Falha ao ordenar a lista por ano.\n");
        return -1;
    }
    return 0;
}


int commandRemove(char *model) {
    int result = rlist_remove_by_model(rlist, model);
    if (result == -1) {
        printf("ERRO : Falha ao remover o carro com modelo '%s'.\n", model);
        return -1;
    }
    if (result == 0) {
        printf("Nenhum carro encontrado com modelo '%s' para remover.\n", model);
    }
    return 0;
}


int commandSize() {
    int result = rlist_size(rlist);
    if (result == -1) {
        printf("ERRO : Falha ao obter o tamanho da lista.\n");
        return -1;
    }
    printf("Tamanho da lista: %d\n", result);
    return 0;
}


int commandQuit() {
    quitSignal = 1;
    return 0;
}


// vê se os argumentos são válidos (0) ou dá erro (-1)
int handleArguments(int argc, char *argv[]) {
    if (argc < 1) {
        return -1;
    }
    char *command = argv[0];
    char *param = NULL;
    if (argc > 2) {
        param = argv[1];
    }


    if (strcmp(command, "add") == 0) {
        if(!param) {
            printf("ERRO : Tem de haver um argumento 'data' após o comando '%s'.\n", command);
            return -1;
        }
        return commandAdd(param);
    }
    if (strcmp(command, "get_by_marca") == 0) {
        if(!param) {
            printf("ERRO : Tem de haver um argumento 'marca' após o comando '%s'.\n", command);
            return -1;
        }
        return commandGetByMarca(param);
    }
    if (strcmp(command, "get_by_year") == 0) {
        if(!param) {
            printf("ERRO : Tem de haver um argumento 'year' após o comando '%s'.\n", command);
            return -1;
        }
        return commandGetByYear(param);
    }
    if (strcmp(command, "get_model_list") == 0) {
        return commandGetModelList();
    }
    if (strcmp(command, "get_list_ordered_by_year") == 0) {
        return commandGetListOrderedByYear();
    }
    if (strcmp(command, "remove") == 0) {
        if(!param) {
            printf("ERRO : Tem de haver um argumento 'model' após o comando '%s'.\n", command);
            return -1;
        }
        return commandRemove(param);
    }
    if (strcmp(command, "size") == 0) {
        return commandSize();
    }
    if (strcmp(command, "quit") == 0) {
        return commandQuit();
    }

    printf("ERRO : Comando desconhecido '%s'.\n", command);
    return -1;
}


int main(int argc, char *argv[]) {
    //validar argumentos iniciais (list_client address:port)
    if (argc != 2) {
        printf("Uso: list_client address:port\n");
        return -1;
    }

    //establecer ligacao
    rlist = rlist_connect(argv[1]);
    if (!rlist) {
        printf("Erro ao conectar ao servidor %s\n", argv[1]);
        return -1;
    }

    //loop de esperar comandos
    char input[128];

    while (quitSignal == 0) {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nExiting...\n");
            break;
        }

        // remove newline
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "") == 0) {
            continue;
        }

        // Tokenize input
        char *argv2[MAX_ARGS];
        int argc2 = 0;

        char *token = strtok(input, " ");
        while (token != NULL && argc2 < MAX_ARGS) {
            argv2[argc2++] = token;
            token = strtok(NULL, " ");
        }

        int result = handleArguments(argc2, argv2);
    }

    //cleanup
    rlist_disconnect(rlist);
}