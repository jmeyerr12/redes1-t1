#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kermit.h"

int main() {
    const char *interface = "enx001122334455"; // substitua pela sua interface real
    int sock = cria_raw_socket((char*)interface);

    char msg[] = "Oi, servidor!";
    kermit_pckt_t pkt;
    gen_kermit_pckt(&pkt, 1, DATA_TYPE, msg, strlen(msg));

    printf("Enviando pacote...\n");
    send(sock, &pkt, sizeof(pkt), 0);

    close(sock);
    return 0;
}
