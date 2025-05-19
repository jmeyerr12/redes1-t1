#include "cliente.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void desenhar_mapa(posicao_t jogador, int visitado[GRID_SIZE][GRID_SIZE]) {
    system("clear"); // ou "cls" no Windows
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

int main() {
    const char *interface = "enx00e04c534458"; // atualize para sua interface correta
    int soquete = cria_raw_socket((char *)interface);

    posicao_t jogador = {0, 0};
    int visitado[GRID_SIZE][GRID_SIZE] = {{0}};

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

        // Envia movimento
        kermit_pckt_t pacote;
        gen_kermit_pckt(&pacote, 0, tipo_mov, NULL, 0);
        send(soquete, &pacote, sizeof(pacote), 0);

        // Espera resposta do servidor
        kermit_pckt_t resposta;
        ssize_t len = recv(soquete, &resposta, sizeof(resposta), 0);
        if (len > 0 && valid_kermit_pckt(&resposta) && !error_detection(&resposta)) {
            if (resposta.type == ACK_TYPE) {
                switch (tipo_mov) {
                    case MOVER_CIMA:    if (jogador.y < GRID_SIZE - 1) jogador.y++; break;
                    case MOVER_BAIXO:   if (jogador.y > 0) jogador.y--; break;
                    case MOVER_ESQ:     if (jogador.x > 0) jogador.x--; break;
                    case MOVER_DIR:     if (jogador.x < GRID_SIZE - 1) jogador.x++; break;
                }
                visitado[jogador.x][jogador.y] = 1;
            }
        }

        usleep(100000); // evitar uso excessivo da CPU
    }

    close(soquete);
    return 0;
}
