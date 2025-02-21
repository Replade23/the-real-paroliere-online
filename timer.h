#ifndef TIMER_H
#define TIMER_H

extern pthread_mutex_t game_mutex; // Mutex per sincronizzazione
extern int game_duration;

// Funzioni per il timer e la pausa
void *game_cycle(void *arg);
void matrix_handler(const char *matrix_file_name);
void t_send(int client_socket);

#endif