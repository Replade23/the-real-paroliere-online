// Struttura del messaggio inviato nella bacheca
typedef struct messaggio
{
    char *messaggio;
    char *mittente;
} Messaggio;

extern int messaggi_inseriti;
// Funzione per inserire messaggio nella bacheca
void inserisci_messaggio(char *messaggio, char *mittente);
// Funzione per leggere la bacheca
Messaggio *leggi_messaggi(int *num_messaggi);
// Funzione per liberare i messaggi dalla memoria
void libera_messaggi(Messaggio *messaggi, int num_messaggi);

void print_bacheca(const char *to_print);

char *messaggi_to_string(Messaggio *messaggi, int num_messaggi);