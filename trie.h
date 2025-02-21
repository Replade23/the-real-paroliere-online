#ifndef TRIE_H
#define TRIE_H

#include <stdbool.h> // Per i tipi booleani
#include <stdio.h>   // Per FILE*

#define NUM_CHAR 26 // Numero di caratteri A-Z

// Definizione della struttura Trie
typedef struct node
{
    struct node *figli[NUM_CHAR]; // Puntatori ai figli
    int is_word;                  // Flag per indicare se il nodo è la fine di una parola
} Trie;

// Prototipi delle funzioni
Trie *create_node();
Trie *load_dictionary(FILE *dizionario);
int insert_Trie(Trie *root, char *word);
int search_Trie(char *word, Trie *trie);
void print_Trie(Trie *trie, char *buffer, int depth);

#endif // TRIE_H