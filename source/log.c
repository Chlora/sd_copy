#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include "sdmessage.pb-c.h"
#include "../include/log.h"
#include <errno.h>

const char *dir = "../log";
const char *name = "server.log";

char* OpcodeToString(MessageT__Opcode opcode) {
    switch (opcode) {
        case MESSAGE_T__OPCODE__OP_ADD:
            return "OP_ADD";
        case MESSAGE_T__OPCODE__OP_GET:
            return "OP_GET";
        case MESSAGE_T__OPCODE__OP_DEL:
            return "OP_DEL";
        case MESSAGE_T__OPCODE__OP_SIZE:
            return "OP_SIZE";
        case MESSAGE_T__OPCODE__OP_GETMODELS:
            return "OP_GETMODELS";
        case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR:
            return "OP_GETLISTBYTEAR";
        case MESSAGE_T__OPCODE__OP_ORDER:
            return "OP_ORDER";
        case MESSAGE_T__OPCODE__OP_ERROR:
            return "OP_ERROR";
        default:
            return "OP_UNKNOWN";
    }
}

char* CTypeToString(MessageT__CType ctype) {
    switch (ctype) {
        case MESSAGE_T__C_TYPE__CT_BAD:
            return "CT_BAD";
        case MESSAGE_T__C_TYPE__CT_DATA:
            return "CT_DATA";
        case MESSAGE_T__C_TYPE__CT_MARCA:
            return "CT_MARCA";
        case MESSAGE_T__C_TYPE__CT_YEAR:
            return "CT_YEAR";
        case MESSAGE_T__C_TYPE__CT_MODEL:
            return "CT_MODEL";
        case MESSAGE_T__C_TYPE__CT_RESULT:
            return "CT_RESULT"; 
        case MESSAGE_T__C_TYPE__CT_LIST:
            return "CT_LIST";
        case MESSAGE_T__C_TYPE__CT_NONE:
            return "CT_NONE";
        default:
            return "CT_UNKNOWN";
    }
}

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

FILE* CreateFile(void) {

    if (mkdir(dir, 0777) == -1 && errno != EEXIST) {
        printf("[ERRO] Erro ao criar diretório de logs.\n");
        return NULL;
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "%s%s%s",
             dir,
             (dir[strlen(dir) - 1] == '/' ? "" : "/"),
             name);

    FILE *file = fopen(filename, "a");
    if (!file) {
        printf("[ERRO] Não foi possível abrir/criar o ficheiro de logs.\n");
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

    //fprintf(file, "%ld.%06ld ", (long)log->tv->tv_sec, log->tv->tv_usec); // com milisegundo
    fprintf(file, "%ld ", (long)log->tv->tv_sec);
    fprintf(file, "%s ", log->client); // address

    switch (log->EventType) {
        case CONNECT:
            fprintf(file, "CONNECT\n");
            break;
        case REQUEST:
            fprintf(file, "REQUEST ");

            fprintf(file, "%s ", OpcodeToString(log->opcode));

            fprintf(file, "%s %s", CTypeToString(log->ctype), log->content);

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