#ifndef LOG_MODULE_H
#define LOG_MODULE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

// Configurazioni globali
#define MAX_USERS 32
#define USERNAME_LENGTH 11

// Struttura per rappresentare un utente
typedef struct
{
    char username[USERNAME_LENGTH];
    int punteggio_tot;
    int client_socket;
    struct sockaddr_in client_address;
} User;

// Struttura per i punteggi del file di Log
typedef struct
{
    char parola[20];
    int punteggio;
} LogEntry;

// Lista dinamica per le entry di log
typedef struct
{
    LogEntry *entries; // Array dinamico di log
    int size;          // Numero di elementi
    int capacity;      // Capacità massima
} LogList;

// Variabili globali
extern User users[MAX_USERS];
extern pthread_mutex_t user_mutex;
extern int n_users;
extern LogList log_list[MAX_USERS];

// Funzioni globali
int register_user(const char *username);
int login_user(const char *username);
int logout_user(const char *username);
int delete_user(const char *username, char *client_username);
int is_user_valid(const char *username);
void send_message(int client_socket, char type, const char *data);
void add_log_entry(LogList *log_list, const char *parola, int punteggio);
void lshiftl(int index);
void write_on_log(const char *input);

#endif