#include "servidor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


int main() {
    const char *interface = "enx00e04c534458"; // atualize com sua interface de rede
    int soquete = cria_raw_socket((char *)interface);

    tesouro_t tesouros[NUM_TESOUROS];
    posicao_t jogador = {0, 0}; // começa no canto inferior esquerdo

    inicializar_tesouros(tesouros);

    printf("[SERVIDOR] Aguardando comandos do cliente...\n");

    while (1) {
        kermit_pckt_t pacote_recebido;
        ssize_t len = recv(soquete, &pacote_recebido, sizeof(pacote_recebido), 0);
        if (len <= 0 || !valid_kermit_pckt(&pacote_recebido)) continue;

        if (error_detection(&pacote_recebido)) {
            printf("[SERVIDOR] Erro no pacote recebido. Ignorando...\n");
            continue;
        }

        int mov = pacote_recebido.type;
        mostrar_log(jogador, "Movimento recebido");

        // Atualiza posição
        switch (mov) {
            case MOVER_CIMA:
                if (jogador.y < GRID_SIZE - 1) jogador.y++;
                break;
            case MOVER_BAIXO:
                if (jogador.y > 0) jogador.y--;
                break;
            case MOVER_DIR:
                if (jogador.x < GRID_SIZE - 1) jogador.x++;
                break;
            case MOVER_ESQ:
                if (jogador.x > 0) jogador.x--;
                break;
            default:
                continue;
        }

        char arquivo[MAX_NOME_ARQ];
        if (verificar_tesouro(tesouros, jogador, arquivo)) {
            mostrar_log(jogador, "Tesouro encontrado!");
            // TODO: Ler e enviar conteúdo do arquivo (em pacotes)
        } else {
            mostrar_log(jogador, "Nada encontrado.");
            kermit_pckt_t ack;
            gen_kermit_pckt(&ack, 0, ACK_TYPE, NULL, 0);
            send(soquete, &ack, sizeof(ack), 0);
        }

        usleep(100000); // evita uso alto de CPU
    }

    close(soquete);
    return 0;
}

void inicializar_tesouros(tesouro_t tesouros[NUM_TESOUROS]) {
    srand(time(NULL));
    int ocupados[GRID_SIZE][GRID_SIZE] = {0};

    for (int i = 0; i < NUM_TESOUROS; i++) {
        int x, y;
        do {
            x = rand() % GRID_SIZE;
            y = rand() % GRID_SIZE;
        } while (ocupados[x][y]);
        ocupados[x][y] = 1;

        tesouros[i].x = x;
        tesouros[i].y = y;
        tesouros[i].encontrado = 0;

        snprintf(tesouros[i].arquivo, MAX_NOME_ARQ, "%d", i + 1); // por ora, só .txt
    }

    printf("[SERVIDOR] Tesouros sorteados:\n");
    for (int i = 0; i < NUM_TESOUROS; i++) {
        printf("  %d: (%d, %d) => %s\n", i + 1, tesouros[i].x, tesouros[i].y, tesouros[i].arquivo);
    }
}

int verificar_tesouro(tesouro_t tesouros[NUM_TESOUROS], posicao_t pos, char *arquivo_encontrado) {
    for (int i = 0; i < NUM_TESOUROS; i++) {
        if (tesouros[i].x == pos.x && tesouros[i].y == pos.y && !tesouros[i].encontrado) {
            strcpy(arquivo_encontrado, tesouros[i].arquivo);
            tesouros[i].encontrado = 1;
            return 1;
        }
    }
    return 0;
}

void mostrar_log(posicao_t pos, const char *evento) {
    printf("[LOG] Jogador em (%d, %d): %s\n", pos.x, pos.y, evento);
}
