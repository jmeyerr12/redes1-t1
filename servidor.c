#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kermit.h"

int main() {
    const char *interface = "enx001122334455"; // substitua pela sua interface real
    int sock = cria_raw_socket((char*)interface);

    printf("Servidor pronto para receber pacotes...\n");

    while (1) {
        kermit_pckt_t pkt;
        ssize_t len = recv(sock, &pkt, sizeof(pkt), 0);
        if (len > 0 && valid_kermit_pckt(&pkt)) {
            printf("Pacote recebido!\n");
            print_kermit_pckt(&pkt);

            if (!error_detection(&pkt)) {
                printf("Checksum OK.\n");
            } else {
                printf("Erro no pacote!\n");
            }
        }

        usleep(100000); // evita 100% de uso da CPU
    }

    close(sock);
    return 0;
}
