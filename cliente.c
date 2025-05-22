#include "cliente.h"

int socket_fd;
int seq = 0;
Posicao posicao_jogador = {0, 0};

void desenhar_mapa(Posicao jogador) {
    printf("\n==== MAPA ====\n");
    for (int y = GRID_SIZE - 1; y >= 0; y--) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (jogador.x == x && jogador.y == y) {
                printf(" X ");
            } else {
                printf(" . ");
            }
        }
        printf("\n");
    }
    printf("================\n\n");
}

void enviar_movimento(char comando) {
    kermit_pckt_t pkt;
    byte_t tipo;

    switch (comando) {
        case 'd': tipo = MOVER_DIR; break;
        case 'w': tipo = MOVER_CIMA; break;
        case 's': tipo = MOVER_BAIXO; break;
        case 'a': tipo = MOVER_ESQ; break;
        default: printf("Comando inválido!\n"); return;
    }

    gen_kermit_pckt(&pkt, seq++, tipo, NULL, 0);
    sendto_rawsocket(socket_fd, &pkt, sizeof(pkt));
}

void receber_arquivo() {
    kermit_pckt_t pkt;
    char buffer[BUF_SIZE];
    char nome_arquivo[64] = {0};
    int tipo = -1;
    int total_bytes = 0;
    int tamanho_esperado = -1;

    FILE *fp = NULL;

    while (1) {
        int bytes = recvfrom_rawsocket(socket_fd, TIMEOUT_MS, buffer, BUF_SIZE);
        if (bytes <= 0) continue;

        memcpy(&pkt, buffer, sizeof(pkt));
        if (!valid_kermit_pckt(&pkt)) continue;

        switch (pkt.type) {
            case TEXT_ACK_NAME:
            case IMG_ACK_NAME:
            case VIDEO_ACK_NAME:
                tipo = pkt.type;
                memcpy(nome_arquivo, pkt.data, pkt.size);
                nome_arquivo[pkt.size] = '\0';
                printf("Recebendo arquivo: %s\n", nome_arquivo);
                if (tipo != TEXT_ACK_NAME) {
                    fp = fopen(nome_arquivo, "wb");
                    if (!fp) {
                        perror("Erro ao abrir arquivo");
                        return;
                    }
                }
                break;

            case TAM_TYPE:
                memcpy(&tamanho_esperado, pkt.data, sizeof(int));
                printf("Tamanho: %d bytes\n", tamanho_esperado);
                break;

            case DATA_TYPE:
                if (tipo == TEXT_ACK_NAME) {
                    pkt.data[pkt.size] = '\0';
                    printf("%s", pkt.data);
                } else if (fp) {
                    fwrite(pkt.data, 1, pkt.size, fp);
                    total_bytes += pkt.size;
                }
                break;

            case END_FILE_TYPE:
                printf("\nFim do arquivo recebido.\n");
                if (fp) fclose(fp);
                return;

            default: break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <interface>\n", argv[0]);
        return EXIT_FAILURE;
    }

    socket_fd = cria_raw_socket(argv[1]);
    printf("Cliente iniciado. Use W/A/S/D para mover. Q para sair.\n");

    while (1) {
        printf("Posição atual: (%d, %d) > ", posicao_jogador.x, posicao_jogador.y);
        desenhar_mapa(posicao_jogador);
        char comando = getchar();
        while (getchar() != '\n'); // limpar buffer

        if (comando == 'q') break;

        enviar_movimento(comando);
        switch (comando) {
            case 'w': if (posicao_jogador.y < GRID_SIZE - 1) posicao_jogador.y++; break;
            case 's': if (posicao_jogador.y > 0) posicao_jogador.y--; break;
            case 'a': if (posicao_jogador.x > 0) posicao_jogador.x--; break;
            case 'd': if (posicao_jogador.x < GRID_SIZE - 1) posicao_jogador.x++; break;
        }

        receber_arquivo();
    }

    close(socket_fd);
    return 0;
}
