#include "cliente.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void desenhar_mapa(posicao_t jogador, int visitado[GRID_SIZE][GRID_SIZE]) {
    system("clear"); 
    printf("=== MAPA 8x8 ===\n");
    for (int y = GRID_SIZE - 1; y >= 0; y--) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (jogador.x == x && jogador.y == y)
                printf(" @ ");
            else if (visitado[x][y])
                printf(" . ");
            else
                printf(" - ");
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <interface>\n", argv[0]);
        exit(1);
    }

    const char *interface = argv[1];
    int soquete = cria_raw_socket((char *)interface);

    posicao_t jogador = {0, 0};
    int visitado[GRID_SIZE][GRID_SIZE] = {{0}};
    int seq = 0;

    printf("[CLIENTE] Use W/A/S/D para mover. Q para sair.\n");

    while (1) {
        desenhar_mapa(jogador, visitado);
        printf("Mover (w/a/s/d): ");
        char input;
        scanf(" %c", &input);

        int tipo_mov = -1;
        switch (input) {
            case 'w': tipo_mov = MOVER_CIMA; break;
            case 's': tipo_mov = MOVER_BAIXO; break;
            case 'a': tipo_mov = MOVER_ESQ; break;
            case 'd': tipo_mov = MOVER_DIR; break;
            case 'q': close(soquete); exit(0);
            default: continue;
        }

        // Envia o movimento
        kermit_pckt_t pacote;
        gen_kermit_pckt(&pacote, seq, tipo_mov, NULL, 0);
        sendto_rawsocket(soquete, &pacote, sizeof(pacote));

        // Espera resposta do servidor
        kermit_pckt_t resposta;
        ssize_t len = recvfrom_rawsocket(soquete, &resposta, sizeof(resposta));
        if (len > 0 && valid_kermit_pckt(&resposta) && !error_detection(&resposta)) {
            if (resposta.type == ACK_TYPE && resposta.seq == seq) {
                switch (tipo_mov) {
                    case MOVER_CIMA:    if (jogador.y < GRID_SIZE - 1) jogador.y++; break;
                    case MOVER_BAIXO:   if (jogador.y > 0) jogador.y--; break;
                    case MOVER_ESQ:     if (jogador.x > 0) jogador.x--; break;
                    case MOVER_DIR:     if (jogador.x < GRID_SIZE - 1) jogador.x++; break;
                }
                visitado[jogador.x][jogador.y] = 1;
                seq = (seq + 1) % 32;
            } else if (resposta.type == NACK_TYPE && resposta.seq == seq) {
                printf("[CLIENTE] NACK recebido (seq %d). Movimento não realizado.\n", seq);
            }
        } else {
            printf("[CLIENTE] Timeout ou pacote inválido. Nenhuma resposta do servidor.\n");
        }

        usleep(100000);
    }

    close(soquete);
    return 0;
}
