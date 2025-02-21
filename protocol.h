#define MSG_OK 'K'              // Conferma generica
#define MSG_ERR 'E'             // Errore generico
#define MSG_REGISTRA_UTENTE 'R' // Registrazione di un utente
#define MSG_CANCELLA_UTENTE 'D' // Cancella utente
#define MSG_LOGIN_UTENTE 'L'    // Login Utente
#define MSG_MATRICE 'M'         // Richiesta/Invio della matrice
#define MSG_TEMPO_PARTITA 'T'   // Invio del tempo rimanente per la partita
#define MSG_TEMPO_ATTESA 'A'    // Invio del tempo di attesa tra le partite
#define MSG_PAROLA 'W'          // Invio di una parola
#define MSG_PUNTI_FINALI 'F'    // Invio dei punteggi finali della partita
#define MSG_PUNTI_PAROLA 'P'    // Punti assegnati a una parola
#define MSG_SERVER_SHUTDOWN 'B' // Comunicazione di arresto del server
#define MSG_POST_BACHECA 'H'    // Pubblicazione di un messaggio in bacheca
#define MSG_SHOW_BACHECA 'S'    // Visualizzazione della bacheca

#include "log.h"

// Struttra usata per memorizzare i socket dei client connessi, così da potervi inviare messaggi in broadcast
typedef struct
{
    int sockets[32]; // Array per memorizzare i socket
    int count;       // Numero di socket attualmente nella lista
} BroadcastList;

extern BroadcastList list;
extern User vincitore;

void send_message(int socket, char type, const char *data);
void send_all();