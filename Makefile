CC = gcc
CFLAGS = -Wall -O2
DEPS = kermit.h raw_socket.h
OBJ_COMUM = kermit.o raw_socket.o

all: servidor cliente

servidor: main_servidor.o $(OBJ_COMUM)
	$(CC) $(CFLAGS) -o servidor main_servidor.o $(OBJ_COMUM)

cliente: main_cliente.o $(OBJ_COMUM)
	$(CC) $(CFLAGS) -o cliente main_cliente.o $(OBJ_COMUM)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o servidor cliente

.PHONY: all clean servidor cliente
