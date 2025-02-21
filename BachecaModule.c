#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "bacheca.h"

#define MAX_MESSAGES 8
#define MAX_STRING_SIZE 2048
// Definisco la grandezza della bacheca
Messaggio bacheca[MAX_MESSAGES];

// Inizializzo variabili globali e mutex
int messaggi_inseriti = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Funzione per inserire messaggio nella bacheca
void inserisci_messaggio(char *messaggio, char *mittente)
{
    pthread_mutex_lock(&lock);

    // Rimuoviamo il messaggio più vecchio se abbiamo raggiunto il limite massimo
    if (messaggi_inseriti >= MAX_MESSAGES)
    {
        free(bacheca[0].messaggio);
        free(bacheca[0].mittente);

        // Spostiamo i messaggi rimanenti verso il basso
        for (int i = 1; i < MAX_MESSAGES; i++)
        {
            bacheca[i - 1] = bacheca[i];
        }
        // Diminuiamo il conteggio dei messaggi inseriti
        messaggi_inseriti--;
    }

    // Inserisco il nuovo messaggio e mittente nella bacheca
    bacheca[messaggi_inseriti].messaggio = malloc(strlen(messaggio) + 1);
    strcpy(bacheca[messaggi_inseriti].messaggio, messaggio);

    bacheca[messaggi_inseriti].mittente = malloc(strlen(mittente) + 1);
    strcpy(bacheca[messaggi_inseriti].mittente, mittente);
    // Aumento il conteggio dei messaggi inseriti
    messaggi_inseriti++;

    pthread_mutex_unlock(&lock);
}

// Funzione per leggere la bacheca
Messaggio *leggi_messaggi(int *num_messaggi)
{
    pthread_mutex_lock(&lock);

    // Alloco memoria per l'array di messaggi da restituire
    Messaggio *messaggi_to_read = malloc(messaggi_inseriti * sizeof(Messaggio));
    // Controllo se c'è almeno un messaggio che è stato postato
    if (messaggi_to_read == NULL)
    {
        printf("Bacheca vuota\n");
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    // Copio i messaggi dalla bacheca
    for (int i = 0; i < messaggi_inseriti; i++)
    {
        messaggi_to_read[i].messaggio = malloc(strlen(bacheca[i].messaggio) + 1);
        strcpy(messaggi_to_read[i].messaggio, bacheca[i].messaggio);

        messaggi_to_read[i].mittente = malloc(strlen(bacheca[i].mittente) + 1);
        strcpy(messaggi_to_read[i].mittente, bacheca[i].mittente);
    }
    pthread_mutex_unlock(&lock);
    // Invio l'array di messaggi
    return messaggi_to_read;
}

// Funzione per liberare i messaggi dalla memoria
void libera_messaggi(Messaggio *messaggi, int num_messaggi)
{
    for (int i = 0; i < num_messaggi; i++)
    {
        free(messaggi[i].messaggio);
        free(messaggi[i].mittente);
    }
    free(messaggi);
}

// Funzione per stampare la bacheca su terminale, prende una stringa come parametro
void print_bacheca(const char *to_print)
{
    char buffer[MAX_STRING_SIZE];
    strncpy(buffer, to_print, MAX_STRING_SIZE - 1);
    buffer[MAX_STRING_SIZE - 1] = '\0'; // Assicuriamo che sia terminata

    // Usa un puntatore per iterare attraverso il buffer
    char *line = buffer;
    while (line && *line != '\0')
    {
        // Trova la posizione del newline
        char *newline = strchr(line, '\n');
        if (newline)
        {
            *newline = '\0'; // Termina la riga corrente
        }

        // Copia temporanea per dividere mittente e messaggio
        char temp_line[MAX_STRING_SIZE];
        strncpy(temp_line, line, MAX_STRING_SIZE - 1);
        temp_line[MAX_STRING_SIZE - 1] = '\0';

        // Estrai mittente e messaggio
        char *mittente = strtok(temp_line, "|");
        char *messaggio = strtok(NULL, "|");

        printf("%s: %s,\n", mittente, messaggio); // Stampa in formato richiesto
        // Vai alla riga successiva
        if (newline)
        {
            line = newline + 1;
        }
        else
        {
            break; // Fine del buffer
        }
    }
}

// Fuznione che converte l'array di messaggi in stringa per inviare tramite socket
char *messaggi_to_string(Messaggio *messaggi, int num_messaggi)
{
    char *to_print = malloc(MAX_STRING_SIZE * num_messaggi);
    if (!to_print)
    {
        perror("Errore allocazione memoria");
        return NULL;
    }

    to_print[0] = '\0'; // Inizializza la stringa vuota

    for (int i = 0; i < num_messaggi; i++)
    {
        char temp[MAX_STRING_SIZE];
        snprintf(temp, MAX_STRING_SIZE, "%s|%s\n", messaggi[i].mittente, messaggi[i].messaggio);
        strncat(to_print, temp, MAX_STRING_SIZE * num_messaggi - strlen(to_print) - 1);
    }

    return to_print;
}