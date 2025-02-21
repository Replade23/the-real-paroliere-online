#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "protocol.h"
#include "timer.h"
#include "matrix.h"
#include "trie.h"
#include "bacheca.h"
#include "log.h"

#define MAX_CLIENTS 32
#define BUFFER_SIZE 1024
#define MATRIX_SIZE 4

char server_name[256] = "localhost";
int server_port = 8080;
int random_seed = 0; // Nessun seed predefinito
char dizionario[256] = "";

Trie *trie_root = NULL;

// Funzione che verrà chiamata alla ricezione di un segnale
void handle_signal(int signal)
{
    if (signal == SIGINT)
    {
        printf("\nSegnale SIGINT ricevuto (Ctrl+C). Chiusura del server...\n");
    }
    else if (signal == SIGTERM)
    {
        printf("\nSegnale SIGTERM ricevuto. Chiusura del server...\n");
    }

    // Esegui eventuali operazioni di pulizia qui (es. chiudi socket, libera memoria, ecc.)
    printf("Eseguo operazioni di cleanup...\n");
    // qui inserire for che chiude tutti i client
    // close(socket_fd);
    for (int i = 0; i < list.count; i++)
    {
        send_message(list.sockets[i], MSG_SERVER_SHUTDOWN, NULL);
        close(list.sockets[i]); // Cleanup dei client
    }

    // fprintf(log_file, "Server chiuso correttamente.\n");
    // fclose(log_file);

    // Termina il programma
    exit(0);
}

// Inizializzatore della lista broadcast
void init_broadcast_list(BroadcastList *list)
{
    list->count = 0; // Inizializza la lista come vuota
}

// Funzione per aggiungere un socket alla lista
int add_socket(BroadcastList *list, int socket)
{
    if (list->count < MAX_CLIENTS)
    {
        list->sockets[list->count] = socket; // Aggiungi il socket nell'array
        list->count++;                       // Incrementa il contatore
        return 0;                            // Successo
    }
    else
    {
        printf("Errore: la lista di socket è piena.\n");
        return -1; // Errore: la lista è piena
    }
}

// Funzione per rimuovere un socket dalla lista
int remove_socket(BroadcastList *list, int socket)
{
    int found = 0;
    for (int i = 0; i < list->count; i++)
    {
        if (list->sockets[i] == socket)
        {
            found = 1;
        }
        if (found && i < list->count - 1)
        {
            list->sockets[i] = list->sockets[i + 1]; // Shifta gli altri socket
        }
    }
    if (found)
    {
        list->count--; // Decrementa il contatore
        return 0;      // Successo
    }
    printf("Errore: socket non trovato nella lista.\n");
    return -1; // Errore: socket non trovato
}

int handle_word(char *word, const char *username, int client_socket)
{
    for (int i = 0; word[i]; i++)
    {
        word[i] = toupper((unsigned char)word[i]);
    }

    pthread_mutex_lock(&user_mutex);
    // Trova il giocatore, faccio un secondo controllo per verificare l'effettiva registrazione
    int user_index = -1;
    for (int i = 0; i < n_users; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            user_index = i;
            break;
        }
    }

    if (user_index == -1)
    {
        pthread_mutex_unlock(&user_mutex);
        send_message(client_socket, MSG_ERR, "Utente non registrato.");
        return -1;
    }

    // Controlla se la parola è già stata inviata
    for (int j = 0; j < log_list[user_index].size; j++)
    {
        if (strcmp(log_list[user_index].entries[j].parola, word) == 0)
        {
            pthread_mutex_unlock(&user_mutex);
            send_message(client_socket, MSG_PUNTI_PAROLA, "0"); // Nessun punteggio per parole duplicate
            printf("parola già usata dal client, punteggio 0\n");
            return 0;
        }
    }

    // Controlla se la parola è nel dizionario
    int root_result = search_Trie((char *)word, trie_root);

    if (root_result != 0)
    {
        pthread_mutex_unlock(&user_mutex);
        send_message(client_socket, MSG_ERR, "Parola non valida nel dizionario.\n");
        return -1;
    }

    // Controlla se la parola è nella matrice
    if (!is_word_in_matrix(matrix, word))
    {
        pthread_mutex_unlock(&user_mutex);
        send_message(client_socket, MSG_ERR, "Parola non presente nella matrice.\n");
        return -1;
    }

    // Calcola il punteggio (considerando "Qu" come una sola lettera)
    int score = 0;
    for (int i = 0; word[i] != '\0'; i++)
    {
        if (word[i] == 'Q' && word[i + 1] == 'U')
        {
            score++;
            i++; // Salta "U"
        }
        else
        {
            score++;
        }
    }

    // Qui aggiorno il vincitore
    // prendo il nome utente se il suo punteggio è il più alto, dovrei scorrere la lista, nella quale dovrei avere una variabile che mi registra ogni parola validata
    for (int i = 0; i < n_users; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            users[i].punteggio_tot += score;
            printf("punteggio attuale: %d\n", users[i].punteggio_tot);

            if (&vincitore == NULL)
            {
                vincitore = users[i];
                printf("il vincitore è %s\n", vincitore.username);
                break;
            }
            if (vincitore.punteggio_tot < users[i].punteggio_tot)
            {

                vincitore = users[i];
                printf("il nuovo vincitore è %s\n", vincitore.username);
                break;
            }
        }
    }

    // Qui invece stampo sul file di log ogni parola che viene passata all'handler
    for (int i = 0; i < list.count; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            char *log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Username: %s | Parola: %s | Punteggio: %d.\n", users[i].username, word, score);
            write_on_log(log_msg);
        }
    }

    // Aggiungi la parola e il punteggio al log
    add_log_entry(&log_list[user_index], word, score);

    // Invia il punteggio al client
    char *score_str[16];
    snprintf(score_str, sizeof(score_str), "%d\n", score);
    send_message(client_socket, MSG_PUNTI_PAROLA, score_str);

    pthread_mutex_unlock(&user_mutex);

    return score;
}

void *handle_client(void *arg)
{
    User *client_data = (User *)arg;
    int client_socket = client_data->client_socket;
    char buffer[BUFFER_SIZE];

    printf("Nuovo client connesso: %s:%d\n",
           inet_ntoa(client_data->client_address.sin_addr),
           ntohs(client_data->client_address.sin_port));

    // Ricezione messaggi: sono in ascolto
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);

        if (bytes_received <= 0)
        {
            int found = 0;
            remove_socket(&list, client_socket);
            printf("Client disconnesso.\n");
            close(client_socket);
            free(client_data);

            pthread_exit(NULL);
        }

        // Controllo dimensione minima
        if (bytes_received < 5)
        {
            printf("Messaggio ricevuto troppo corto.\n");
            close(client_socket);
            free(client_data);
            pthread_exit(NULL);
        }

        // Estrarre i componenti del messaggio
        char type = buffer[0]; // Il primo byte è il tipo del messaggio
        uint32_t length;
        memcpy(&length, buffer + 1, sizeof(uint32_t)); // Leggi la lunghezza (4 byte)
        length = ntohl(length);                        // Converte da network byte order a host byte order

        // Controllo limite massimo
        if (length > BUFFER_SIZE - 5)
        {
            printf("Lunghezza messaggio non valida.\n");
            close(client_socket);
            free(client_data);
            pthread_exit(NULL);
        }
        // Estrarre i dati (se presenti)
        char *data = NULL;
        if (length > 0)
        {
            data = malloc(length + 1); // Alloca spazio per i dati
            if (!data)
            {
                perror("Errore di allocazione memoria");
                close(client_socket);
                free(client_data);
                pthread_exit(NULL);
            }
            memcpy(data, buffer + 1 + sizeof(uint32_t), length);
            data[length] = '\0'; // Terminatore di stringa
        }

        char response;
        // Switch per gestire i tipi di messaggi
        switch (type)
        {
        case MSG_REGISTRA_UTENTE: // Registrazione utente
            if (register_user(data) == 0)
            {
                strncpy(client_data->username, data, USERNAME_LENGTH - 1);
                client_data->username[USERNAME_LENGTH - 1] = '\0'; // Assicurati che sia terminato
                send_message(client_socket, MSG_OK, "Utente registrato con successo");
                char *log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "%s registrato!\n", client_data->username);
                write_on_log(log_msg);
            }
            else
            {
                send_message(client_socket, MSG_ERR, "Nome utente già in uso\n");
            }
            break;

        case MSG_LOGIN_UTENTE: // Login Utente
            login_user(data) == 0 ? send_message(client_socket, MSG_LOGIN_UTENTE, "Utente loggato con successo\n") : send_message(client_socket, MSG_ERR, "Nome utente non trovato o già loggato\n");
            break;

        case MSG_CANCELLA_UTENTE: // Cancellazione utente
            if (delete_user(data, client_data->username) == 0)
            {
                send_message(client_socket, MSG_CANCELLA_UTENTE, "Utente eliminato con successo\n");
                char *log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "%s cancellato!\n", client_data->username);
                write_on_log(log_msg);
                break;
            }
            if (delete_user(data, client_data->username) == -2)
            {
                int res = (delete_user(data, client_data->username) == -2);
                send_message(client_socket, MSG_ERR, "Puoi cancellare solo il tuo username!\n");
                break;
            }
            else
            {
                send_message(client_socket, MSG_ERR, "Nome utente non trovato\n");
                break;
            }
        case MSG_MATRICE: // Richiesta di stampa della matrice
            m_send(client_socket);
            break;

        case MSG_TEMPO_PARTITA: // Richiesta tempo rimanente
            t_send(client_socket);
            break;

        case MSG_SHOW_BACHECA: // Mostra la bacheca
            Messaggio *msg_list = leggi_messaggi(messaggi_inseriti);
            char *to_print = messaggi_to_string(msg_list, messaggi_inseriti);
            send_message(client_socket, MSG_SHOW_BACHECA, to_print);
            break;

        case MSG_POST_BACHECA: // Posta sulla la bacheca
            printf("il messaggio è: %s\n da %s\n", data, client_data->username);
            inserisci_messaggio(data, client_data->username);
            send_message(client_socket, MSG_OK, "Messaggio pubblicato su bacheca\n");
            break;

        case MSG_PAROLA: // Invio la Parola al Server
            if (handle_word(data, client_data->username, client_socket) < 0)
            {
                send_message(client_socket, MSG_ERR, "Errore durante la validazione della parola.\n");
            }
            break;

        default: // Tipo sconosciuto
            printf("Tipo di messaggio sconosciuto: %c\n", type);
            send(client_socket, "Comando non riconosciuto.\n", 27, 0);
            break;
        }

        // Libera la memoria allocata per i dati
        if (data)
        {
            free(data);
        }
    }
}

int main(int argc, char *argv[])
{
    // Imposta i gestori per SIGINT e SIGTERM
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    init_broadcast_list(&list);

    char buffer[BUFFER_SIZE];
    int server_socket;
    struct sockaddr_in server_address;

    static struct option long_options[] = {
        {"matrici", required_argument, 0, 'm'},
        {"durata", required_argument, 0, 'd'},
        {"seed", required_argument, 0, 's'},
        {"diz", required_argument, 0, 'z'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int opt;

    // Controllo che siano state scritte la porta del server e il nome
    if (argc < 3)
    {
        fprintf(stderr, "Uso: %s nome_server porta_server [opzioni]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Salva i primi due argomenti (porta e nome server)
    strncpy(server_name, argv[1], sizeof(server_name) - 1);
    server_port = atoi(argv[2]);

    // Carico gli altri parametri
    while (
        (opt = getopt_long(argc - 2, argv + 2, "m:d:s:z:", long_options, &option_index)) != -1)
    {
        printf("\n---- carico %c ----", (char)(opt));
        switch (opt)
        {
        case 'm': // Nome file contenente matrici
            printf("Parsing (M) %s\n\n", optarg);
            matrix_file_name = strdup(optarg);
            break;
        case 'd': // Durata in secondi delle partite
            printf("Parsing (D) %s\n\n", optarg);
            game_duration = atoi(optarg);
            break;
        case 's': // Seed per generazione matrice random
            printf("Parsing (S) %s\n\n", optarg);
            random_seed = atoi(optarg);
            printf("Random Seed: %d", random_seed);
            break;
        case 'z': // Nome File dizionario
            printf("Parsing (Z) %s\n\n", optarg);
            strncpy(dizionario, optarg, strlen(optarg) + 1);
            break;
        default:
            fprintf(stderr, "Opzione sconosciuta.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Stampo i parametri per verificare cosa ho caricato
    printf("Nome Server: %s\n", server_name);  // Nome Server
    printf("Porta Server: %d\n", server_port); // Porta Server

    if (strlen(matrix_file_name) > 0) // File Matrice
    {
        printf("File Matrici: %s\n", matrix_file_name);
    }
    printf("Durata Partita: %d secondi\n", game_duration); // Durata Partita
    printf("Seed Random: %d\n", random_seed);              // Seed per generazione matrice random
    if (strlen(dizionario) > 0)                            // File Dizionario
    {
        printf("File Dizionario: %s\n", dizionario);
    }

    FILE *file_dizionario = fopen(dizionario, "r");
    // Controllo che il dizionario sia caricato e in caso scelgo quello di default
    file_dizionario = fopen("dizionario.txt", "r");
    if (!file_dizionario)
    {
        printf("Errore caricamento dizionario, utilizzo %s come default \n\n", dizionario);
        strncpy(dizionario, "dizionario.txt", 15);
        trie_root = load_dictionary(file_dizionario);
    }
    else
    {
        printf("Dizionario caricato correttamente utilizzando %s \n\n", dizionario);
        trie_root = load_dictionary(file_dizionario);
    }

    // Creazione del socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Errore creazione socket");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);

    // Binding
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Errore binding");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("Errore listen");
        exit(EXIT_FAILURE);
    }

    // Iniziallizza la mutex
    // pthread_mutex_init(&game_mutex, NULL);

    // Inizializza il thread per la gestione del tempo di gioco
    pthread_t timer_thread;
    pthread_create(&timer_thread, NULL, game_cycle, &game_duration);

    printf("Server in ascolto sulla porta %d...\n", server_port);

    while (1)
    {
        User *client_data = malloc(sizeof(User));
        socklen_t addr_len = sizeof(client_data->client_address);

        // Accept
        if ((client_data->client_socket = accept(
                 server_socket,
                 (struct sockaddr *)&client_data->client_address, &addr_len)) < 0)
        {
            perror("Errore accept");
            free(client_data);
            continue;
        }

        add_socket(&list, client_data->client_socket);

        /* for (int i = 0; i < list.count; i++)
        {
            printf("\nlista socket: %d\n", list.sockets[i]);
        } */

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client_data);
        pthread_detach(thread_id);
    }

    return 0;
}
