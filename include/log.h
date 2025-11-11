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
    Cria um ficheiro de texto vazio com o nome especificado por 'name', seguido da data (ao segundo) em que foi criado.
    Se não for especificado um nome, o default é 'syslog'.

    Retorna o ponteiro para o ficheiro ou NULL em caso de erro.
*/
FILE* CreateFile(char* path, char* name);


/*
    Escreve uma entrada de log no ficheiro especificado.

    Retorna 0 em caso de sucesso ou -1 em caso de erro.
    Liberta a memória de ServerLog.
*/
int WriteLog(struct ServerLog *log, FILE* file);


/*
    Fecha o ficheiro, libertando a sua memória ocupada.

    Retorna 0 em caso de sucesso ou -1 em caso de erro.
*/
int CloseFile(FILE *file);


#endif