# Variabili
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread

# File sorgenti
SRCS_SERVER = GameServer.c LogModule.c MatrixModule.c protocol.c TimeModule.c Trie.c BachecaModule.c
SRCS_CLIENT = GameClient.c LogModule.c MatrixModule.c protocol.c TimeModule.c Trie.c BachecaModule.c

# File oggetto
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)

# Esegui server e client
TARGET_SERVER = server
TARGET_CLIENT = client

# Regola di compilazione
all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_SERVER) $(OBJS_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT)

rebuild: clean all
