#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"

#define MAX_USERS 32       // Numero massimo di utenti
#define USERNAME_LENGTH 11 // Lunghezza massima per il nome utente (+1 per \0)

User users[MAX_USERS];                                  // Lista di utenti
int n_users = 0;                                        // numero di utenti
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex per sincronizzazione
LogList log_list[MAX_USERS];

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funzione che permette di registrare un determinato username (a patto che non sia già stato registrato)
int register_user(const char *username)
{
    pthread_mutex_lock(&user_mutex);

    if (n_users == MAX_USERS)
    {
        pthread_mutex_unlock(&user_mutex);
        return -2; // Lista utenti piena
    }

    for (int i = 0; i < n_users; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            pthread_mutex_unlock(&user_mutex);
            return -1; // Nome già in uso
        }
    }

    strncpy(users[n_users].username, username, USERNAME_LENGTH - 1);
    users[n_users].username[USERNAME_LENGTH - 1] = '\0'; // Assicurati che sia terminato

    n_users++; // Incremento il numero di utenti

    pthread_mutex_unlock(&user_mutex);

    return 0; // Registrazione riuscita
}

// Funzione che permette il login di un determinato username (a patto che sia precedentemente registrato)
int login_user(const char *username)
{
    pthread_mutex_lock(&user_mutex);

    for (int i = 0; i < n_users; i++)
    {
        if (strcmp(users[i].username, username) == 0 /* && !users[i].active */)
        {
            // Alloca la lista log per l'utente al login
            log_list[i].entries = malloc(10 * sizeof(LogEntry));
            log_list[i].size = 0;
            log_list[i].capacity = 10;

            // Inserisce il nome utente come prima entry
            strcpy(log_list[i].entries[log_list[i].size].parola, username);
            log_list[i].entries[log_list[i].size].punteggio = 0; // Nessun punteggio associato al nome utente
            log_list[i].size++;

            pthread_mutex_unlock(&user_mutex);
            printf("login effettuato, utente %s connesso!\n", username);
            return 0; // Login effettuato con successo
        }
    }

    pthread_mutex_unlock(&user_mutex);
    printf("Utente '%s' già loggato o nome utente errato\n", username);
    return -1; // Nome utente non trovato o già attivo
}

// Funzione chiamata quando l'utente rimane afk per un determinato tempo (AFK_TREASHOLD)
int logout_user(const char *username)
{
    for (int i = 0; i < n_users; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            // users[i].active = false;
            pthread_mutex_unlock(&user_mutex);
            return 0; // Logout effettuato con successo
        }
    }

    pthread_mutex_unlock(&user_mutex);
    return -1; // Nome Utente non trovato
}

// Funzione che viene chiamata per eliminare uno user, prende in argomento username dell'utente connesso e quello da cancellare
int delete_user(const char *username, char *client_username)
{
    pthread_mutex_lock(&user_mutex);

    if (strcmp(username, client_username) == 0)
    {
        for (int i = 0; i < n_users; i++)
        {
            if (strcmp(users[i].username, username) == 0)
            {
                lshiftl(i);
                n_users--;

                pthread_mutex_unlock(&user_mutex);
                return 0; // Utente eliminato con successo
            }
        }
        pthread_mutex_unlock(&user_mutex);
        return -1; // Nome Utente non trovato
    }
    pthread_mutex_unlock(&user_mutex);
    return -2; // Nome utente da cancellare e nome utente loggato non corrispondono
}

// Funzione che permette di cancellare un utente dalla lista users tramite shift logico
void lshiftl(int index)
{
    for (int i = index + 1; i < n_users; i++)
    {
        users[i - 1] = users[i];
    }
}

// Per controllare se il nome utente è valido
int is_user_valid(const char *username)
{
    for (int i = 0; username[i] != '\0'; i++)
    {
        if (!((username[i] >= 'A' && username[i] <= 'Z') ||
              (username[i] >= 'a' && username[i] <= 'z') ||
              (username[i] >= '1' && username[i] <= '9')))
        {
            return 0; // Non valido
        }
    }
    return 1; // Valido
}

// Inizializzo la lista log
void init_log_list(LogList *log_list, const char *username)
{
    log_list->entries = malloc(10 * sizeof(LogEntry)); // Capacità iniziale
    log_list->size = 0;
    log_list->capacity = 10;

    strcpy(log_list->entries[log_list->size].parola, username);
    log_list->entries[log_list->size].punteggio = 0;
    log_list->size++;
}

// Aggiunge un'entry della lista
void add_log_entry(LogList *log_list, const char *parola, int punteggio)
{
    if (log_list->size == log_list->capacity)
    {
        log_list->capacity *= 2; // Raddoppia la capacità se necessario
        log_list->entries = realloc(log_list->entries, log_list->capacity * sizeof(LogEntry));
    }
    strcpy(log_list->entries[log_list->size].parola, parola);
    log_list->entries[log_list->size].punteggio = punteggio;
    log_list->size++;
}

// Funzione che permette, passato un input in formato string, di stampare quell'input nel file di log
void write_on_log(const char *input)
{
    printf("input: %s", input);
    // Verifica che i parametri non siano nulli
    if (input == NULL)
    {
        fprintf(stderr, "Errore: nome_file o input nulli.\n");
        return;
    }

    // Blocco mutex per garantire l'accesso esclusivo al file
    pthread_mutex_lock(&log_mutex);

    // Apri il file in modalità "append" (aggiunta, non sovrascrittura)
    FILE *log_file = fopen("log.txt", "a");
    if (log_file == NULL)
    {
        perror("Errore nell'apertura del file di log");
        pthread_mutex_unlock(&log_mutex); // Sblocca la mutex in caso di errore
        return;
    }

    // Scrivi l'input nel file
    fprintf(log_file, "%s\n", input);

    // Chiudi il file
    fclose(log_file);

    // Sblocca la mutex
    pthread_mutex_unlock(&log_mutex);
}