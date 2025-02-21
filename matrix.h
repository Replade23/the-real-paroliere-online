#ifndef MATRIX
#define MATRIX

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define MATRIX_SIZE 4 // Dimensione della matrice

extern char *matrix_file_name;
extern FILE *matrix_file;
extern char *file_name;

extern char matrix[MATRIX_SIZE][MATRIX_SIZE];

// Funzioni
void open_file(const char *filename);
int next_row(char *buffer, size_t buffer_size, char matrix[MATRIX_SIZE][MATRIX_SIZE]);
void generate_random_matrix(char matrix[MATRIX_SIZE][MATRIX_SIZE]);
void print_matrix(char matrix[MATRIX_SIZE][MATRIX_SIZE]);
int m_send(int client_socket);
int is_word_in_matrix(char matrix[MATRIX_SIZE][MATRIX_SIZE], const char *word);

#endif