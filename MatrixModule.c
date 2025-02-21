#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/socket.h>
#include "matrix.h"
#include "protocol.h"

char *matrix_file_name = NULL; // Nome del file di matrici
FILE *matrix_file = NULL;      // File globale per leggere le matrici
long file_position = 0;        // Posizione corrente nel file
char matrix[MATRIX_SIZE][MATRIX_SIZE];

char caratteri[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', '@', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

// Funzione per aprire il file e che ritorna errore se il file non è presente;
// nel main c'è una chiamata a open_file e prende il nome del file dal comando di avvio del server
void open_file(const char *matrix_file_name)
{
    matrix_file = fopen(matrix_file_name, "r");
    if (!matrix_file)
    {
        perror("Errore apertura file");
    }
    file_position = 0; // Inizia dalla prima posizione
}

// Funzione che legge la riga successiva del file;
// chiamata quando si aggiorna la matrice o all'avvio
int next_row(char *buffer, size_t buffer_size, char matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    if (!matrix_file)
    {
        fprintf(stderr, "File non aperto.\n");
        return -1;
    }

    // Vai alla posizione salvata
    if (fseek(matrix_file, file_position, SEEK_SET) != 0)
    {
        perror("Errore durante il seek");
        return -1;
    }

    // Leggi la riga
    if (!fgets(buffer, buffer_size, matrix_file))
    {
        if (feof(matrix_file))
        {
            // Se siamo alla fine del file, riparti dall'inizio
            rewind(matrix_file);
            file_position = 0;                            // Resetta la posizione
            return next_row(buffer, buffer_size, matrix); // Riprova
        }
        else
        {
            perror("Errore durante la lettura del file");
            return -1;
        }
    }

    // Salva la nuova posizione
    file_position = ftell(matrix_file);

    size_t index = 0;
    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            matrix[i][j] = buffer[index]; // Assegna il carattere corrente
            index += 2;                   // Salta lo spazio successivo (ogni lettera è seguita da uno spazio)
        }
    }

    return 0;
}

// Funzione per generare la matrice randomicamente se non viene fornito il file contente le matrici
void generate_random_matrix(char matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    srand(time(NULL));

    char temp[26]; // Array temporaneo per contenere i caratteri selezionati
    char row[16];  // Array contenente 16 caratteri randomizzati
    memcpy(temp, caratteri, 26);

    // Mescola l'array con Fisher-Yates Shuffle
    for (int i = 25; i > 0; i--)
    {
        int j = rand() % (i + 1);
        char temp_char = temp[i];
        temp[i] = temp[j];
        temp[j] = temp_char;
    }

    // Seleziona i primi 16 caratteri randomizzati e li salva in un'array a parte
    for (int k = 15; k >= 0; k--)
    {
        row[k] = temp[k];
    }

    // Trasferisce i caratteri nella matrice 4x4
    size_t index = 0;
    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            matrix[i][j] = row[index++];
        }
    }
}

// Funzione per stampare la matrice
void print_matrix(char matrix[MATRIX_SIZE][MATRIX_SIZE])
{

    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            if (matrix[i][j] == 'Q' || matrix[i][j] == '@')
            {
                printf("Qu ");
            }
            else
            {
                printf("%c ", matrix[i][j]);
            }
        }
        printf("\n");
    }
}

// Direzioni possibili per il movimento
int directions[8][2] = {
    {-1, 0}, {1, 0}, {0, -1}, {0, 1}, // Su, giù, sinistra, destra
    {-1, -1},
    {-1, 1},
    {1, -1},
    {1, 1} // Diagonali sx giù, diagonale sx sù, diagonale dx giù, diagonale dx sù
};

// Funzione per verificare se una cella è valida (la chiama solo is_word_in_matrix)
int is_matrix_valid(int x, int y, int visited[MATRIX_SIZE][MATRIX_SIZE])
{
    return x >= 0 && x < MATRIX_SIZE && y >= 0 && y < MATRIX_SIZE && !visited[x][y];
}

// Backtracking per trovare la parola (la chiama solo is_word_in_matrix)
int find_word_in_matrix(char matrix[MATRIX_SIZE][MATRIX_SIZE], int x, int y, const char *word, int index, int visited[MATRIX_SIZE][MATRIX_SIZE])
{
    if (index == strlen(word))
    {
        return 1; // Parola trovata
    }

    if (!is_matrix_valid(x, y, visited) || matrix[x][y] != word[index])
    {
        return 0; // Posizione non valida o lettera non corrisponde
    }

    visited[x][y] = 1; // Marca la cella come visitata

    // Prova tutte le direzioni
    for (int d = 0; d < 8; d++)
    {
        int new_x = x + directions[d][0];
        int new_y = y + directions[d][1];

        if (find_word_in_matrix(matrix, new_x, new_y, word, index + 1, visited))
        {
            return 1;
        }
    }

    visited[x][y] = 0; // Backtracking: libera la cella
    return 0;
}

// Funzione principale per controllare se la parola è nella matrice
int is_word_in_matrix(char matrix[MATRIX_SIZE][MATRIX_SIZE], const char *word)
{
    int visited[MATRIX_SIZE][MATRIX_SIZE] = {0};

    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            if (find_word_in_matrix(matrix, i, j, word, 0, visited))
            {
                return 1;
            }
        }
    }

    return 0; // Parola non trovata
}

// Funzione per inviare la matrice al Client
int m_send(int client_socket)
{
    // Prepara il messaggio secondo il protocollo
    char type = MSG_MATRICE;                                           // Tipo di messaggio
    unsigned int length = MATRIX_SIZE * MATRIX_SIZE;                   // Dimensione fissa per 4x4
    char buffer[1 + sizeof(unsigned int) + MATRIX_SIZE * MATRIX_SIZE]; // Spazio per type, length e dati

    buffer[0] = type;                                          // Primo byte: type
    memcpy(buffer + 1, &length, sizeof(unsigned int));         // Successivi 4 byte: length
    memcpy(buffer + 1 + sizeof(unsigned int), matrix, length); // Successivi: dati

    // Invio del messaggio al client
    int bytes_sent = send(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_sent < 0)
    {
        perror("Errore durante l'invio della matrice\n");
        return -1;
    }

    printf("Matrice inviata correttamente al client.\n");
    return 0;
}
