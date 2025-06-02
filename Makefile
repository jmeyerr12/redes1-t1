CC = gcc
CFLAGS = -Wall -O2
DEPS = kermit.h raw_socket.h
OBJ_COMUM = kermit.o raw_socket.o

all: servidor cliente

servidor: servidor.o $(OBJ_COMUM)
	$(CC) $(CFLAGS) -o servidor servidor.o $(OBJ_COMUM)

cliente: cliente.o $(OBJ_COMUM)
	$(CC) $(CFLAGS) -o cliente cliente.o $(OBJ_COMUM)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o servidor cliente *.txt *.jpg *.mp4

.PHONY: all clean servidor cliente
