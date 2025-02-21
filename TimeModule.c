#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "timer.h"
#include "matrix.h"
#include "protocol.h"
#include "log.h"

// Definizione delle variabili globali
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;
int game_running = 0; // Flag che indica se la partita è in corso
time_t start_time = 0;
int game_duration;

#define PAUSE_BETWEEN_GAMES 60 // Pausa tra le partite in secondi
#define BUFFER_SIZE 1024

int time_remaining(int game_duration)
{
    if (!game_running)
    {
        printf("Game is not running!\n");
        return 0; // Nessuna partita in corso
    }

    time_t current_time;
    time(&current_time); // Ottieni il tempo corrente

    int elapsed_time = difftime(current_time, start_time);      // Tempo trascorso
    int remaining_time = difftime(game_duration, elapsed_time); // Tempo Rimanente

    return remaining_time > 0 ? remaining_time : 0; // Ritorna il tempo rimanente, oppure 0 se è scaduto
}

void matrix_handler(const char *matrix_file_name)
{
    char buffer[256];

    // Controlla se il nome del file è nullo
    if (matrix_file_name == NULL)
    {
        printf("Nessun file specificato. Generazione di una matrice casuale...\n");
        generate_random_matrix(matrix);
        print_matrix(matrix); // Stampa matrice casuale
        return;
    }

    // Tenta di aprire il file
    /* open_file(matrix_file_name);

    // Verifica se il file è stato aperto correttamente
    if (matrix_file == NULL)
    {
        printf("Errore durante l'apertura del file %s. Generazione di una matrice casuale...\n", matrix_file_name);
        generate_random_matrix(matrix);
        print_matrix(matrix); // Stampa matrice casuale
        return;
    } */

    // Legge una riga dal file e la trasforma in matrice
    if (next_row(buffer, sizeof(buffer), matrix) == 0)
    {
        print_matrix(matrix); // Stampa matrice letta dal file
    }
    else
    {
        printf("Errore durante la lettura del file. Generazione di una matrice casuale...\n");
        generate_random_matrix(matrix);
        print_matrix(matrix); // Stampa matrice casuale
    }
}
void t_send(int client_socket)
{
    char string[1024];
    // Prepare the message content
    char type = MSG_TEMPO_PARTITA;                         // Message type
    unsigned int time_rem = time_remaining(game_duration); // Compute the remaining time

    // Prepare the string to send (example: format as text for debugging, or customize protocol)
    unsigned int length = sizeof(unsigned int);

    if (length >= sizeof(string))
    {
        fprintf(stderr, "Error: Message too long\n");
        return;
    }

    // Create the buffer with type, length, and data
    char buffer[1 + sizeof(unsigned int) + sizeof(time_rem)];
    buffer[0] = type;                                                           // First byte: type
    memcpy(buffer + 1, &length, sizeof(unsigned int));                          // Next 4 bytes: length
    memcpy(buffer + 1 + sizeof(unsigned int), &time_rem, sizeof(unsigned int)); // Next bytes: data

    // Send the message to the client
    int bytes_sent = send(client_socket, buffer, 1 + sizeof(unsigned int) + sizeof(time_rem), 0);

    if (bytes_sent < 0)
    {
        perror("Send error");
        return;
    }

    printf("Tempo rimanente inviato al client socket: %d\n", client_socket);
}

// Funzione per gestire il timer della partita
void *game_cycle(void *arg)
{
    int game_duration = *(int *)arg;

    open_file(matrix_file_name);

    // Verifica se il file è stato aperto correttamente
    if (matrix_file == NULL)
    {
        printf("Errore durante l'apertura del file %s. Generazione di una matrice casuale...\n", matrix_file_name);
        generate_random_matrix(matrix);
        print_matrix(matrix); // Stampa matrice casuale
        return;
    }

    while (1)
    {

        // Inizio della partita
        pthread_mutex_lock(&game_mutex);
        game_running = 1;
        pthread_mutex_unlock(&game_mutex);
        printf("stato partita: %d\n", game_running);
        // Qua inizializzo la lista per il file di log

        for (int i = 0; i < list.count; i++)
        {
            send_message(list.sockets[i], MSG_OK, "inizio partita");
        }

        printf("\n== Nuova partita iniziata! Durata: %d secondi ==\n", game_duration);

        matrix_handler(matrix_file_name);

        // Timer per la partita
        time_t current_time;
        time(&start_time); // reset del timer della partita (usato anche per capire il tempo rimanente)

        do
        {
            time(&current_time);
        } while (difftime(current_time, start_time) < game_duration);

        printf("== Partita terminata! ==\n");

        // Invia_fine_partita() invierà un codice di fine partita a tutti i giocatori connessi, loro visualizzeranno il punteggio
        send_all();

        for (int i = 0; i < list.count; i++)
        {
            char *log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Username: %s | Punteggio: %d.\n", users[i].username, users[i].punteggio_tot);
            write_on_log(log_msg);
        }

        // Fine della partita

        pthread_mutex_lock(&game_mutex);
        game_running = 0;
        pthread_mutex_unlock(&game_mutex);
        printf("stato partita: %d\n", game_running);
        // Pausa tra le partite
        printf("== Pausa tra le partite: %d secondi ==\n", PAUSE_BETWEEN_GAMES);
        sleep(PAUSE_BETWEEN_GAMES);
    }

    return NULL;
}