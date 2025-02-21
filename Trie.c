#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "trie.h"

// Funzione che carica il dizionario
Trie *load_dictionary(FILE *dizionario)
{
    Trie *root = create_node();
    char word[256];

    // Legge ogni parola dal file e la inserisce nel Trie
    while (fscanf(dizionario, "%s", word) == 1)
    {
        for (int i = 0; word[i]; i++)
        {
            word[i] = toupper((unsigned char)word[i]);
        }
        insert_Trie(root, word);
    }

    fclose(dizionario);
    printf("dizionario creato correttamente\n");
    return root;
}

// Funzione che crea un nodo del Trie
Trie *create_node()
{
    // alloco il nodo
    Trie *trie = (Trie *)malloc(sizeof(Trie));
    // inizializzo i valori della radice
    trie->is_word = -1;
    // inizializzo i figli
    for (int i = 0; i < NUM_CHAR; i++)
    {
        trie->figli[i] = NULL;
    }
    return trie;
}

// Funzione per cercare se la parola si trova nel trie
int search_Trie(char *word, Trie *trie)
{
    if (trie == NULL)
    {
        printf("Nodo corrente è NULL: la parola '%s' non è presente.\n", word);
        return 0;
    }

    if (*word == '\0')
    {
        printf("Fine della parola raggiunta. is_word=%d\n", trie->is_word);
        return trie->is_word;
    }

    int index = *word - 'A';
    if (index < 0 || index >= NUM_CHAR)
    {
        printf("Carattere '%c' non valido trovato. Indice=%d\n", *word, index);
        return -1; // Carattere non valido
    }

    if (trie->figli[index] == NULL)
    {
        printf("Nodo figlio per carattere '%c' non trovato. Parola '%s' non presente.\n", *word, word);
        return -1;
    }

    return search_Trie(word + 1, trie->figli[index]);
}

// Funzione per inserire parola nel trie
int insert_Trie(Trie *root, char *word)
{
    if (root == NULL)
    {
        root = create_node();
    }

    if (*word == '\0')
    {
        root->is_word = 0;
        return 0;
    }

    int target_child = *word - 'A';
    if (target_child < 0 || target_child >= NUM_CHAR)
        return -1; // Carattere non valido

    if (root->figli[target_child] == NULL)
    {
        root->figli[target_child] = create_node();
    }

    return insert_Trie(root->figli[target_child], word + 1);
}

// Funzione di debug, stampa il trie in modo da visualizzare come è composto
void print_Trie(Trie *trie, char *buffer, int depth)
{
    // caso base
    if (trie == NULL)
    {
        printf("l'albero è vuoto\n");
        return;
    }
    // caso base
    if (trie->is_word == 0)
    {
        buffer[depth] = '\0';
        printf("%s\n", buffer);
    }
    // faccio una ricerca per trovare il prossimo carattere su cui ricorrere
    for (int i = 0; i < NUM_CHAR; i++)
    {
        if (trie->figli[i] != NULL)
        {
            buffer[depth] = i + 'A';
            print_Trie(trie->figli[i], buffer, depth + 1);
        }
    }
    return;
}

/*
int main()
{
    FILE *dizionario = fopen("dizionario.txt", "r");
    if (!dizionario)
    {
        perror("Errore nell'apertura del dizionario");
        return EXIT_FAILURE;
    }

    Trie *trie_root = load_dictionary(dizionario);

    char parola[256];
    printf("Inserisci una parola da controllare: ");
    scanf("%255s", parola);

    for (int i = 0; parola[i]; i++)
    {
        parola[i] = toupper(parola[i]); // Converte in maiuscolo
    }

    int risultato = search_Trie(parola, trie_root);

    if (risultato == 0)
    {
        printf("La parola '%s' è presente nel dizionario.\n", parola);
    }
    else
    {
        printf("La parola '%s' NON è presente nel dizionario.\n", parola);
    }

    // Libera la memoria del Trie (aggiungi la funzione free_trie se necessario)
    return 0;
} */