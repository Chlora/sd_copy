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
    } else {
        printf("ERRO : Marca desconhecida '%s'.\n", marca);
        return -1;
    }

    rlist_get_by_marca(rlist, marca_enum);
}


int commandGetByYear(char *year) {
    int year_int = atoi(year);
    rlist_get_by_year(rlist, year_int);
}


int commandGetModelList() {
    rlist_get_model_list(rlist);
}


int commandGetListOrderedByYear() {
    return rlist_order_by_year(rlist);
}


int commandRemove(char *model) {
    return rlist_remove_by_model(rlist, model);
}


int commandSize() {
    return rlist_size(rlist);
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