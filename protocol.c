#include "protocol.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#define BUFFER_SIZE 1024

// Viene aggiornato ogni volta che invio una parola da client e resettato all'inizio delle partite
User vincitore;

BroadcastList list;

void send_message(int client_socket, char type, const char *data)
{
    uint32_t length = data ? strlen(data) : 0; // Lunghezza dei dati
    uint32_t length_n = htonl(length);         // Conversione a network byte order

    char buffer[5 + length]; // 1 byte per il tipo, 4 byte per la lunghezza, resto per i dati
    buffer[0] = type;
    memcpy(buffer + 1, &length_n, sizeof(length_n)); // Copia lunghezza in network byte order
    if (data)
    {
        memcpy(buffer + 5, data, length); // Copia i dati
    }

    // printf("Sending message: Type=%c, Length=%d, Data=%s\n", type, length, data ? data : "NULL");
    send(client_socket, buffer, 5 + length, 0);
}

void send_all()
{
    char result[256];
    snprintf(result, sizeof(result), "Vincitore: %s | Punteggio Finale: %d\n", vincitore.username, vincitore.punteggio_tot);
    for (int i = 0; i <= list.count; i++)
    {
        send_message(list.sockets[i], MSG_PUNTI_FINALI, result);
    }
    write_on_log(result);
}
