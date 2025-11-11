/**
 * @file log.h
 * @brief Interface para um sistema de logs simples
 *
 * Este ficheiro define as funções necessárias para a
 * escrita de eventos do sistema num ficheiro de texto.
 *
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autores:
 * Data: 10/11/2025
 */

#ifndef SERVERLOG_H
#define SERVERLOG_H /* Módulo */

#include <sys/file.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "sdmessage.pb-c.h"

enum EventType {
    CONNECT, 
    REQUEST, 
    CLOSE
};

struct ServerLog {
    struct timeval *tv;
    char* client;
    enum EventType EventType;
    MessageT__Opcode opcode;
    MessageT__CType ctype;
    char* content;
    char** argument;
};

/*
    Obtém o lock para o ficheiro ./log/server.log. Se não existir diretoria/ficheiro, cria-os.

    Retorna o ponteiro para o ficheiro ou NULL em caso de erro.
*/
FILE* CreateFile();


/*
    Escreve uma entrada de log no ficheiro especificado.
    Liberta a memória dos componentes de ServerLog, mas não liberta a memória da estrutura em si.

    Retorna 0 em caso de sucesso ou -1 em caso de erro.
*/
int WriteLog(struct ServerLog *log, FILE* file);


/*
    Liberta a memória ocupada pela estrutura ServerLog.

    Retorna 0 em caso de sucesso ou -1 em caso de erro.
*/
int FreeServerLog(struct ServerLog *log);


/*
    Fecha o ficheiro, libertando a sua memória ocupada.

    Retorna 0 em caso de sucesso ou -1 em caso de erro.
*/
int CloseFile(FILE *file);


#endif