#include "cliente.h"

int socket_fd;
int seq = 0;
Posicao posicao_jogador = {0, 0};
int mapa[GRID_SIZE][GRID_SIZE] = {0}; // 0 = vazio, 1 = tesouro encontrado

void desenhar_mapa(Posicao jogador) {
    printf("\n==== MAPA ====\n");
    for (int y = GRID_SIZE - 1; y >= 0; y--) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (jogador.x == x && jogador.y == y) {
                printf(" P ");
            } else if (mapa[y][x] == 1) {
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

void receber_arquivo(int tipo, const char *nome_arquivo, int tamanho) {
    FILE *fp = NULL;
    int total_bytes = 0;

    if (tipo != TEXT_ACK_NAME) {
        fp = fopen(nome_arquivo, "wb");
        if (!fp) {
            perror("Erro ao criar arquivo");
            return;
        }
    }

    printf("Recebendo arquivo: %s (%d bytes)\n", nome_arquivo, tamanho);
    mapa[posicao_jogador.y][posicao_jogador.x] = 1; // Marca a posição atual como tendo um tesouro encontrado

    while (1) {
        char buffer[BUF_SIZE];
        kermit_pckt_t *pkt = (kermit_pckt_t *)buffer;

        int bytes = recvfrom_rawsocket(socket_fd, TIMEOUT_MS, buffer, BUF_SIZE);
        if (bytes <= 0 || !valid_kermit_pckt(pkt)) continue;

        if (pkt->type == DATA_TYPE) {
            if (tipo == TEXT_ACK_NAME) {
                pkt->data[pkt->size] = '\0';
                printf("%s", pkt->data);
            } else {
                fwrite(pkt->data, 1, pkt->size, fp);
            }
            total_bytes += pkt->size;
        } else if (pkt->type == END_FILE_TYPE) {
            if (fp) fclose(fp);
            printf("\nArquivo %s recebido (%d bytes).\n", nome_arquivo, total_bytes);
            return;
        }
    }
}

void verificar_resposta() {
    char buffer[BUF_SIZE];
    kermit_pckt_t *pkt = (kermit_pckt_t *)buffer;

    static char nome_arquivo[64] = "";
    static int tipo_arquivo = -1;
    static int aguardando_arquivo = 0;
    static int tamanho_arquivo = -1;

    long long inicio = timestamp();
    while (timestamp() - inicio < 500) {
        int bytes = recvfrom_rawsocket(socket_fd, 50, buffer, BUF_SIZE); // timeout curto
        if (bytes <= 0 || !valid_kermit_pckt(pkt)) continue;

        switch (pkt->type) {
            case OKACK_TYPE:
                printf("Movimento realizado com sucesso (OKACK).\n");
                break;

            case ACK_TYPE:
                printf("Comando reconhecido, mas movimento inválido (ACK).\n");
                break;

            case NACK_TYPE:
                printf("Servidor não recebeu o comando corretamente (NACK).\n");
                break;

            case TEXT_ACK_NAME:
            case IMG_ACK_NAME:
            case VIDEO_ACK_NAME:
                tipo_arquivo = pkt->type;
                memcpy(nome_arquivo, pkt->data, pkt->size);
                nome_arquivo[pkt->size] = '\0';
                aguardando_arquivo = 1;
                responder_ack(OKACK_TYPE, pkt->seq); // confirmar recebimento do nome
                break;

            case TAM_TYPE:
                if (aguardando_arquivo && tipo_arquivo != -1 && nome_arquivo[0] != '\0') {
                    memcpy(&tamanho_arquivo, pkt->data, sizeof(int));
                    responder_ack(OKACK_TYPE, pkt->seq); // confirmar recebimento do tamanho
                    receber_arquivo(tipo_arquivo, nome_arquivo, tamanho_arquivo);
                    tipo_arquivo = -1;
                    aguardando_arquivo = 0;
                    nome_arquivo[0] = '\0';
                    tamanho_arquivo = -1;
                    return;
                }
                break;

            default:
                // Ignora tipos que não fazem sentido neste contexto
                break;
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
        desenhar_mapa(posicao_jogador);
        printf("Posição atual: (%d, %d) > ", posicao_jogador.x, posicao_jogador.y);
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

        verificar_resposta();
    }

    close(socket_fd);
    return 0;
}
