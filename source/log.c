#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include "sdmessage.pb-c.h"
#include "../include/log.h"

int FreeServerLog(struct ServerLog *log) {
    if (log == NULL) {
        return -1;
    }

    free(log->tv);
    free(log->client);
    free(log->content);
    if (log->argument != NULL) {
        for (int i = 0; log->argument[i] != NULL; i++) {
            free(log->argument[i]);
        }
        free(log->argument);
    }

    return 0;
}

FILE* CreateFile(char* path, char* name) {
    if (name == NULL) {
        name = "syslog";
    }

    mkdir(path, 0777);  // fazer diretorio se nao existir

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long timestamp = tv.tv_sec;

    char filename[256];
    snprintf(filename, sizeof(filename), "%s%s%s_%ld.txt",
             path,
             (path[strlen(path)-1] == '/' ? "" : "/"),
             name, timestamp);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erro ao criar o ficheiro de log.\n");
        return NULL;
    }

    return file;
}


int WriteLog(struct ServerLog *log, FILE* file) {
    if (log == NULL || file == NULL) {
        return -1;
    }
    if (log->tv == NULL || log->client == NULL || log->EventType < CONNECT || log->EventType > CLOSE || log->content == NULL) {
        return FreeServerLog(log);
    }

    fprintf(file, "[%ld.%06ld] ", (long)log->tv->tv_sec, log->tv->tv_usec); // timestamp
    fprintf(file, "%s", log->client); // address

    switch (log->EventType) {
        case CONNECT:
            fprintf(file, "CONNECT\n");
            break;
        case REQUEST:
            fprintf(file, "REQUEST ");

            switch (log->opcode) {
            case MESSAGE_T__OPCODE__OP_ADD:
                fprintf(file, "OP_ADD ");
                break;
            case MESSAGE_T__OPCODE__OP_GET:
                fprintf(file, "OP_GET ");
                break;
            case MESSAGE_T__OPCODE__OP_DEL:
                fprintf(file, "OP_DEL ");
                break;
            case MESSAGE_T__OPCODE__OP_SIZE:
                fprintf(file, "OP_SIZE ");
                break;
            case MESSAGE_T__OPCODE__OP_GETMODELS:
                fprintf(file, "OP_GETMODELS ");
                break;
            case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR:
                fprintf(file, "OP_GETLISTBYTEAR ");
                break;
            case MESSAGE_T__OPCODE__OP_ORDER:
                fprintf(file, "OP_ORDER ");
                break;
            case MESSAGE_T__OPCODE__OP_ERROR:
                fprintf(file, "OP_ERROR ");
                break;
            default:
                fprintf(file, "OP_UNKNOWN ");
                break;
            }

            fprintf(file, "%d %s", log->ctype, log->content);

            if (log->argument != NULL) {
                for (int i = 0; log->argument[i] != NULL; i++) {
                    fprintf(file, " %s", log->argument[i]);
                }
            }

            fprintf(file, "\n");
            break;
        case CLOSE:
            fprintf(file, "CLOSE\n");
            break;
        default:
            fprintf(file, "UNKNOWN EVENT\n");
            break;
    }

    
    return FreeServerLog(log);
}

int CloseFile(FILE *file) {
    return fclose(file);
}