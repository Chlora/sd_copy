/**
 * @file list_client.c
 * 
 * @brief Client-side implementation for remote list operations
 * 
 * SD-12
 * @author Rodrigo Antunes - 57879
 * @author Rodrigo Santos - 61825
 * @author Teresa Grangeia - 61869
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "client_stub.h"
#include "client_stub-private.h"
#include "network_client.h"

#include "data.h"
#include "list.h"
#include "list-private.h"

struct rlist_t *rlist = NULL;

#define MAX_ARGS 7
int quitSignal = 0;


int commandAdd(int argc, char **argv) {
    if (argc != 5) {
        printf("Uso: add <modelo> <ano> <preco> <marca> <combustivel>\n");
        printf("Exemplo: add Corolla 2021 25000.50 0 0\n");
        return -1;
    }

    const char *modelo = argv[0];
    int ano = atoi(argv[1]);
    float preco = atof(argv[2]);
    int marca_int = atoi(argv[3]);
    int combustivel_int = atoi(argv[4]);

    if (marca_int < 0 || marca_int > 4) {
        printf("ERRO : Marca inválida '%d'.\n", marca_int);
        printf("Marcas válidas:\n0 - TOYOTA\n1 - BMW\n2 - RENAULT\n3 - AUDI\n4 - MERCEDES\n");
        return -1;
    }
    enum marca_t marca_enum = (enum marca_t) marca_int;

    
    if (combustivel_int < 0 || combustivel_int > 3) {
        printf("ERRO : Combustível inválido '%d'.\n", combustivel_int);
        printf("Combustíveis válidos:\n0 - GASOLINA\n1 - GASOLEO\n2 - ELETRICO\n3 - HIBRIDO\n");
        return -1;
    }
    enum combustivel_t combustivel_enum = (enum combustivel_t) combustivel_int;

    struct data_t *car = data_create(ano, preco, marca_enum, modelo, combustivel_enum);
    if (car == NULL) {
        printf("ERRO : Falha ao criar o carro com os dados fornecidos.\n");
        return -1;
    }

    int result = rlist_add(rlist, car);
    data_destroy(car);

    if (result == -1) {
        printf("ERRO : Falha ao adicionar o carro à lista remota.\n");
        return -1;
    }

    return 0;
}


int commandGetByMarca(int argc, char **argv) {
    if (argc != 1) {
        printf("Uso: get_by_marca <marca>\n");
        return -1;
    }
    int marca_int = atoi(argv[0]);   
    
    if (marca_int < 0 || marca_int > 4) {
        printf("ERRO : Marca inválida '%d'.\n", marca_int);
        printf("Marcas válidas:\n0 - TOYOTA\n1 - BMW\n2 - RENAULT\n3 - AUDI\n4 - MERCEDES\n");
        return -1;
    }
    enum marca_t marca_enum = (enum marca_t) marca_int;

    struct data_t *result = rlist_get_by_marca(rlist, marca_enum);
    if (result == NULL) {
        printf("ERRO : Falha ao obter carro da marca %s.\n", argv[0]);
        return -1;
    }
    printf("Carro da marca %s:\n", argv[0]);
    printf("Modelo: %s, Marca: %d, Ano: %d\n", result->modelo, result->marca, result->ano);
    data_destroy(result);

    return 0;
}


int commandGetByYear(int argc, char **argv) {
    if (argc != 1) {
        printf("Uso: get_by_year <year>\n");
        return -1;
    }

    int year_int = atoi(argv[0]);
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


int commandGetModelList(int argc, char **argv) {
    (void)argc; (void)argv;

    char **arr = rlist_get_model_list(rlist);
    if (arr == NULL) {
        printf("ERRO : Falha ao obter a lista de modelos.\n");
        return -1;
    }
    printf("Lista de modelos:\n");
    for (int i = 0; arr[i] != NULL; i++) {
        printf("%s\n", arr[i]);
    }

    rlist_free_model_list(arr);

    return 0;
}


int commandGetListOrderedByYear(int argc, char **argv) {
    (void)argc; (void)argv;

    int result = rlist_order_by_year(rlist);
    if (result == -1) {
        printf("ERRO : Falha ao ordenar a lista por ano.\n");
        return -1;
    }
    return 0;
}


int commandRemove(int argc, char **argv) {
    if (argc != 1) {
        printf("\tUso: remove <model>\n");
        return -1;
    }

    int result = rlist_remove_by_model(rlist, argv[0]);
    if (result == -1) {
        printf("ERRO : Falha ao remover o carro com modelo '%s'.\n", argv[0]);
        return -1;
    }
    if (result == 1) {
        printf("Nenhum carro encontrado com modelo '%s' para remover.\n", argv[0]);
    }
    return 0;
}


int commandSize(int argc, char **argv) {
    (void)argc; (void)argv; // unused

    int result = rlist_size(rlist);
    if (result == -1) {
        printf("ERRO : Falha ao obter o tamanho da lista.\n");
        return -1;
    }
    printf("Tamanho da lista: %d\n", result);
    return 0;
}


int commandQuit(int argc, char **argv) {
    (void)argc; (void)argv; // unused
    
    quitSignal = 1;
    return 0;
}

static int cmd_help(int argc, char **argv) {
    (void)argc; (void)argv; // unused

    printf("\nComandos:\n");
    printf("add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
    printf("get_by_marca <marca:0-4>\n");
    printf("get_by_year <year>\n");
    printf("get_model_list\n");
    printf("get_list_ordered_by_year\n");
    printf("remove <model>\n");
    printf("size\n");
    printf("quit\n");
    printf("help\n");
    return 0;
}


// vê se os argumentos são válidos (0) ou dá erro (-1)
/* int handleArguments(int argc, char *argv[]) {
    if (argc < 1) {
        return -1;
    }
    char *command = argv[0];
    char *param = NULL;
    if (argc > 1) {
        param = argv[1];
    }


    if (strcmp(command, "add") == 0) {
        if (argc < 6) {
            printf("ERRO : Tem de haver 5 argumentos 'ano', 'preco', 'marca', 'modelo', 'combustivel' após o comando '%s'.\n", command);
            return -1;
        }
        char *data[5];
        for(int i = 0; i < 5; i++){
            data[i] = argv[i+1];
        }
        return commandAdd(data);
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
    if (strcmp(command, "help") == 0) {
        return print_cli();
    }

    printf("ERRO : Comando desconhecido '%s'.\n", command);
    return -1;
} */

typedef int (*cmd_handler_t)(int argc, char **argv);

typedef struct {
    const char *name;
    cmd_handler_t function;
} command_t;

static const command_t commands[] = {
    {"add", commandAdd},
    {"get_by_marca", commandGetByMarca},
    {"get_by_year", commandGetByYear},
    {"get_model_list", commandGetModelList},
    {"get_list_ordered_by_year", commandGetListOrderedByYear},
    {"remove", commandRemove},
    {"size", commandSize},
    {"help", cmd_help},
    {"quit", commandQuit},
    {NULL, NULL} // sentinel
};

static int dispatch_command(int argc, char **argv) {
    if (argc < 1) {
        return -1;
    }
    
    const char *cmd = argv[0];
    
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(cmd, commands[i].name) == 0) {
            return commands[i].function(argc - 1, argv + 1);
        }
    }
    
    printf("ERRO: Comando desconhecido '%s'\n", cmd);
    printf("Digite 'help' para ver os comandos disponíveis\n");
    return -1;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: list_client address:port\n");
        return -1;
    }

    //conectar
    rlist = rlist_connect(argv[1]);
    if (!rlist) {
        printf("Erro ao conectar ao servidor %s\n", argv[1]);
        return -1;
    }

    cmd_help(0, NULL);

    //loop de esperar comandos
    char input[256];
    char *tokens[MAX_ARGS];

    while (quitSignal == 0) {
        printf("\n> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nExiting...\n");
            break;
        }

        //remover newline
        input[strcspn(input, "\n")] = '\0';

        if (input[0] == '\0') continue;

        //tokenizar input
        int token_c = 0; 
        char *token = strtok(input, " \t");
        while (token != NULL && token_c < MAX_ARGS) {
            tokens[token_c++] = token;
            token = strtok(NULL, " \t");
        }

        //executar comando
        int result = dispatch_command(token_c, tokens);

        if (result == -1) {
            printf("Erro ao executar o comando. Quaisquer operações foram descartadas.\n");
        } else {
            printf("Comando executado com sucesso.\n");
        }
    }

    //cleanup
    int result = rlist_disconnect(rlist);
    if (result != 0) {
        printf("Erro ao desconectar do servidor.\n");
        return -1;
    }

    return 0;
}