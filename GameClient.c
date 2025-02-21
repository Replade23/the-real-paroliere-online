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
#include "protocol.h"
#include "matrix.h"
#include "log.h"
#include "timer.h"
#include "bacheca.h"

#define BUFFER_SIZE 1024
#define AFK_THREASHOLD 120

int is_logged_in = 0;
time_t last_action_at;
int game_running_flag = 1;

void handle_server_message(int client_socket)
{
    char type;
    uint32_t length;

    // Ricezione messaggio dal server
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);

    if (bytes_received <= 0)
    {
        if (bytes_received == 0)
        {
            printf("\nConnessione chiusa dal server.\n");
            is_logged_in = 0;
        }
        else
        {
            perror("\nErrore durante la ricezione dal server\n");
        }
        pthread_exit(NULL);
    }

    // Analisi del messaggio
    if (bytes_received < 1 + sizeof(unsigned int))
    {
        printf("\nMessaggio incompleto ricevuto.\n");
        return;
    }

    type = buffer[0];

    if (type == MSG_MATRICE || type == MSG_TEMPO_PARTITA)
    {
        length = ntohl(length);
        memcpy(&length, buffer + 1, sizeof(unsigned int));
    }
    else
    {
        memcpy(&length, buffer + 1, sizeof(unsigned int));
        length = ntohl(length);
    }

    char *data = malloc(length + 1);
    memcpy(data, buffer + 1 + sizeof(unsigned int), length);

    if (length > BUFFER_SIZE - 1)
    {
        printf("Messaggio troppo lungo ricevuto (%u byte).\n", length);
        return;
    }

    data[length] = '\0'; // Aggiungi terminatore di stringa

    // Gestione dei tipi di messaggi
    switch (type)
    {
    case MSG_OK:
        if (strcmp(data, "inizio partita") == 0)
        {
            game_running_flag = 1;
            break;
        }
        printf("%s\n", data);
        break;
    case MSG_ERR:
        if (strcmp(data, "fine partita") == 0)
        {
            game_running_flag = 0;
            break;
        }
        printf("%s\n", data);
        break;
    case MSG_MATRICE:
        printf("matrice: %s\n", data);
        print_matrix(data);
        break;
    case MSG_TEMPO_PARTITA:
        unsigned int tmp;
        memcpy(&tmp, buffer + 1 + sizeof(unsigned int), sizeof(unsigned int));
        printf("Tempo rimanente: %d secondi\n", tmp);
        break;
    case MSG_LOGIN_UTENTE:
        is_logged_in = 1;
        printf("Utente loggato con successo\n");
        break;
    case MSG_CANCELLA_UTENTE:
        is_logged_in = 0;
        printf("Utente eliminato con successo\n");
        break;
    case MSG_PUNTI_PAROLA:
        printf("punti: %s\n", data);
        break;
    case MSG_SHOW_BACHECA:
        print_bacheca(data);
        break;
    case MSG_SERVER_SHUTDOWN:
        printf("Server in chiusura, sarete presto scollegati...\n");
        fflush(stdout);
        exit(0);
        break;
    case MSG_PUNTI_FINALI:
        printf("%s", data);
        if (game_running_flag == 1)
            game_running_flag = 0;
        break;
    default:
        printf("Messaggio sconosciuto ricevuto: '%c'\n", type);
        break;
    }
    free(data);
}

// Funzione che genera il thread per la gestione dei messaggi del server
void *server_thread(void *arg)
{
    int client_socket = *(int *)arg;

    while (1)
    {
        handle_server_message(client_socket);
    }

    return NULL;
}

// Funzione che, tramite thread dedicato, gestisce i comandi dello user
void handle_user_input(int client_socket)
{
    char buffer[BUFFER_SIZE];
    // Lettura input utente
    if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
    {
        printf("Errore nella lettura dell'input.\n");
        pthread_exit(NULL);
    }

    buffer[strcspn(buffer, "\n")] = '\0';
    // Separazione comando e argomento
    char *command = strtok(buffer, " ");
    char *argument = strtok(NULL, "");

    if (command == NULL)
    {
        printf("Comando vuoto. Prova 'help' per la lista comandi.\n");
        return;
    }

    if (difftime(time(NULL), last_action_at) > AFK_THREASHOLD)
    {
        // l'utente ha inviato un comando dopo il tempo di inattività massimo
        is_logged_in = 0;
        printf("\t(Attenzione) Disconnesso per inattività\n");
        last_action_at = time(NULL);
        return; // Se la condizione è vera, esco senza procedere con la lettura dell'input
    }

    // aggiornamento del tempo dell'ultima azione
    last_action_at = time(NULL);

    // Conversione del comando in un identificatore
    int cmd_id = -1;
    if (strcmp(command, "registra") == 0 || strcmp(command, "r") == 0)
        cmd_id = 0;
    else if (strcmp(command, "login") == 0 || strcmp(command, "l") == 0)
        cmd_id = 1;
    else if (strcmp(command, "cancella_registrazione") == 0 || strcmp(command, "cr") == 0)
        cmd_id = 2;
    else if (strcmp(command, "matrice") == 0 || strcmp(command, "m") == 0)
        cmd_id = 3;
    else if (strcmp(command, "parola") == 0 || strcmp(command, "p") == 0)
        cmd_id = 4;
    else if (strcmp(command, "tempo") == 0 || strcmp(command, "t") == 0)
        cmd_id = 5;
    else if (strcmp(command, "msg") == 0)
        cmd_id = 6;
    else if (strcmp(command, "show-msg") == 0)
        cmd_id = 7;
    else if (strcmp(command, "help") == 0)
        cmd_id = 8;

    // Gestione comandi
    switch (cmd_id)
    {
    case 0: // registra
        if (is_logged_in == 0)
        {
            if (argument && strlen(argument) <= 10 && is_user_valid(argument))
            {
                send_message(client_socket, MSG_REGISTRA_UTENTE, argument);
            }
            else
            {
                printf("Errore: il nome utente deve essere lungo al massimo 10 caratteri e contenere solo caratteri alfanumerici.\n");
            }
            break;
        }
        else
            printf("Sei già loggato!\n");
        break;
    case 1: // login
        if (is_logged_in == 0)
        {

            if (argument)
            {
                send_message(client_socket, MSG_LOGIN_UTENTE, argument);
            }
            else
            {
                printf("Errore: specificare un nome utente per il comando 'login'.\n");
            }
            break;
        }
        else
            printf("Sei già loggato!\n");
        break;
    case 2: // cancella_registrazione
        if (is_logged_in == 1)
        {
            if (argument)
            {
                send_message(client_socket, MSG_CANCELLA_UTENTE, argument);
            }
            else
            {
                printf("Errore: specificare un nome utente per il comando 'cancella_registrazione'.\n");
            }
            break;
        }
        else
            printf("Prima di procedere, assicurati di aver effettuato il login\n");
        break;
    case 3: // matrice
        if (is_logged_in == 1)
        {
            if (game_running_flag == 0)
            {
                printf("La partita è in pausa, attendi che ricominci!\n");
                break;
            }
            send_message(client_socket, MSG_MATRICE, NULL);
            break;
        }
        else
        {
            printf("Prima di procedere, assicurati di aver effettuato il login\n");
            break;
        }
    case 4: // parola
        if (is_logged_in == 1)
        {
            if (game_running_flag == 0)
            {
                printf("La partita è in pausa, attendi che ricominci!\n");
                break;
            }
            if (argument)
            {
                send_message(client_socket, MSG_PAROLA, argument);
            }
            else
            {
                printf("Errore: specificare una parola per il comando 'parola'.\n");
            }
            break;
        }
        else
        {
            printf("Prima di procedere, assicurati di aver effettuato il login\n");
            break;
        }
    case 5: // tempo
        if (is_logged_in == 1)
        {
            if (game_running_flag == 0)
            {
                printf("La partita è in pausa, attendi che ricominci!\n");
                break;
            }
            send_message(client_socket, MSG_TEMPO_PARTITA, NULL);
            break;
        }
        else
        {
            printf("Prima di procedere, assicurati di aver effettuato il login\n");
            break;
        }
    case 6: // posta
        if (is_logged_in == 1)
        {
            if (strlen(argument) < 0)
            {
                printf("E' successo qualcosa di inaspettato, lunghezza dell'argomento minore di 0\n");
                break;
            }
            else if (strlen(argument) == 0)
            {
                printf("Specificare un messaggio, massimo 128 caratteri\n");
                break;
            }
            else if (strlen(argument) <= 128)
            {
                printf("sto per inviare il messaggio alla bacheca\n");
                send_message(client_socket, MSG_POST_BACHECA, argument);
                break;
            }
            else
                printf("Errore, massimo consentiti 128 caratteri\n");
            break;
        }
        else
        {
            printf("Prima di procedere, assicurati di aver effettuato il login\n");
            break;
        }

    case 7: // show
        if (is_logged_in == 1)
        {
            send_message(client_socket, MSG_SHOW_BACHECA, NULL);
            break;
        }
        else
        {
            printf("Prima di procedere, assicurati di aver effettuato il login\n");
            break;
        }

    case 8: // help
        if (is_logged_in == 1)
        {
            printf("Comandi disponibili:\n");
            printf(" - registra (r): Registra un nuovo utente\n");
            printf(" - login (l): Effettua il login\n");
            printf(" - cancella_registrazione (cr): Cancella la registrazione\n");
            printf(" - matrice (m): Richiede la matrice\n");
            printf(" - parola (p): Invia una parola\n");
            printf(" - tempo (t): Richiede il tempo della partita\n");
            printf(" - posta (msg): Posta un messaggio sulla bacheca\n");
            printf(" - show (show-msg): Mostra la bacheca\n");
            break;
        }
        else
        {
            printf("Comandi disponibili:\n");
            printf(" - registra (r): Registra un nuovo utente\n");
            printf(" - login (l): Effettua il login\n");
            break;
        }
    default:
        printf("Comando non riconosciuto. Prova 'help' per la lista comandi.\n");
        break;
    }
}

// Funzione che genera il thread per la gestione dei comandi user
void *user_input_thread(void *arg)
{
    int client_socket = *(int *)arg;

    while (1)
    {
        handle_user_input(client_socket);
    }

    return NULL;
}

void main_loop(int client_socket)
{
    pthread_t server_thread_id, user_thread_id;

    // Creazione del thread per la gestione dell'input dell'utente
    if (pthread_create(&user_thread_id, NULL, user_input_thread, &client_socket) != 0)
    {
        perror("Errore durante la creazione del thread per l'input utente\n");
        exit(EXIT_FAILURE);
    }

    // Creazione del thread per la gestione dei messaggi del server
    if (pthread_create(&server_thread_id, NULL, server_thread, &client_socket) != 0)
    {
        perror("Errore durante la creazione del thread per il server\n");
        exit(EXIT_FAILURE);
    }

    // Attendere che i thread terminino
    pthread_join(server_thread_id, NULL);
    pthread_join(user_thread_id, NULL);
}

int main(int argc, char *argv[])
{

    // Controllo che siano passati tutti i parametri, altrimenti non avvio il client
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s nome_server porta_server\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parametri di connessione
    const char *server_name = argv[1];
    int port = atoi(argv[2]);

    // Creazione del socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo del server
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    // Conversione dell'indirizzo IP
    if (inet_pton(AF_INET, server_name, &server_address.sin_addr) <= 0)
    {
        perror("Errore nella conversione dell'indirizzo IP");
        exit(EXIT_FAILURE);
    }

    // Connessione al server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Errore nella connessione al server");
        exit(EXIT_FAILURE);
    }

    printf("Connesso al server %s:%d\n", server_name, port);

    last_action_at = time(NULL);

    // Avvio del ciclo di gestione dei comandi
    main_loop(client_socket);

    // Chiusura della connessione
    close(client_socket);
    return 0;
}
