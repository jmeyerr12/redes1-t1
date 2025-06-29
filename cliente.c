#include "cliente.h"

int socket_fd;
int seq = 0;
Posicao posicao_jogador = {0, 0};
int mapa[GRID_SIZE][GRID_SIZE] = {0}; // 0 = vazio, 1 = tesouro encontrado 2 = visitado
static int quedas = 0;
char *interface;

void desenhar_mapa(Posicao jogador) {
    system("clear");
    printf("\n==== MAPA ====\n");
    for (int y = GRID_SIZE - 1; y >= 0; y--) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (jogador.x == x && jogador.y == y) {
                printf(" P ");
            } else if (mapa[y][x] == 1) {
                printf(" X ");
            } else if (mapa[y][x] == 2) {
                printf(" - ");
            } else {
                printf(" . ");
            }
        }
        printf("\n");
    }
    printf("================\n\n");
    printf("Posição atual: (%d, %d) > ", posicao_jogador.x, posicao_jogador.y);
}

int esperar_ack(kermit_pckt_t *pkt) {
    char ack_buf[BUF_SIZE];
    kermit_pckt_t *resp = (kermit_pckt_t *)ack_buf;

    long inicio = timestamp();
    int quedas = 0;

    while (timestamp() - inicio < 500) {
        sendto_rawsocket(socket_fd, pkt, sizeof(*pkt));

        int bytes = recvfrom_rawsocket(socket_fd, 50, ack_buf, BUF_SIZE);

        if (bytes <= 0) {
            if (++quedas > 100) {
                puts("[CLIENTE] link ausente; reiniciando socket…");
                close(socket_fd);
                socket_fd = cria_raw_socket("enx00e04c534458");
                quedas = 0;
            }
            continue;
        }

        quedas = 0;

        if (!valid_kermit_pckt(resp)) continue;
        if (resp->seq != pkt->seq) continue;

        if (resp->type == OKACK_TYPE || resp->type == ACK_TYPE) return 1;
        if (resp->type == NACK_TYPE) continue;
    }

    return 0; // timeout total estourado
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
    FILE *fp = fopen(nome_arquivo, "wb");
    if (!fp) {
        perror("Erro ao criar arquivo");
        return;
    }

    int total_bytes = 0;
    int percent = 0;
    int prev_percent = 0;
    int ultima_seq = -1; 
    printf("Recebendo arquivo: %s (%d bytes)\n", nome_arquivo, tamanho);
    //int cont = 0;
    while (1) {
        char buffer[BUF_SIZE];
        kermit_pckt_t *pkt = (kermit_pckt_t *)buffer;

        int bytes = recvfrom_rawsocket(socket_fd, TIMEOUT_MS, buffer, BUF_SIZE);
        if (bytes == -1) {
            responder_ack(NACK_TYPE, pkt->seq); //recvfrom_rawsocket ja valida se o pacote veio valido e retorna -1 se tiver algo errado
            continue;
        }
        if (pkt->type == DATA_TYPE) {
            if (pkt->seq == ultima_seq) {
                responder_ack(OKACK_TYPE, pkt->seq); // reenviar ACK para o mesmo pacote
                continue; // ignora gravação duplicada
            }
            
            percent = (total_bytes*100)/tamanho;
            if (percent != prev_percent) {
                system("clear");
                fflush(stdout);
                printf("Recebendo tesouro: %d%%\n", percent);
                fflush(stdout);
            }
            prev_percent = percent;

            responder_ack(OKACK_TYPE, pkt->seq);
            //printf("Gravando %d bytes (seq %d)\n", pkt->size, pkt->seq);
            fwrite(pkt->data, 1, pkt->size, fp);
            total_bytes += pkt->size;
            ultima_seq = pkt->seq;
        } else if (pkt->type == END_FILE_TYPE) {
            responder_ack(OKACK_TYPE, pkt->seq);
            fclose(fp);
            printf("\nArquivo '%s' salvo com sucesso (%d bytes).\n", nome_arquivo, total_bytes);

            char comando[128];
            snprintf(comando, sizeof(comando), "xdg-open \"%s\" &", nome_arquivo);
            system(comando);
            return;
        }
    }
}

void responder_ack(byte_t tipo, byte_t seq) {
    kermit_pckt_t ack;
    gen_kermit_pckt(&ack, seq, tipo, NULL, 0);
    sendto_rawsocket(socket_fd, &ack, sizeof(ack));
}

int arquivo_cabe(const char *caminho, int tamanho_arquivo) {
    struct statvfs stat;
    if (statvfs(caminho, &stat) != 0) {
        return 0; // erro ao obter info
    }

    unsigned long long espaco_livre = stat.f_bsize * stat.f_bavail;

    return espaco_livre >= (unsigned long long)tamanho_arquivo;
}


int verificar_resposta() {
    char  buffer[BUF_SIZE];
    kermit_pckt_t *pkt = (kermit_pckt_t *)buffer;

    /* estado para download de arquivo */
    static char nome_arquivo[64] = "";
    static int  tipo_arquivo     = -1;
    static int  aguardando_arquivo = 0;
    static int  tamanho_arquivo    = -1;

    long long inicio = timestamp();

    while (timestamp() - inicio < 2000)              /* janela total 2000 ms */
    {
        int bytes = recvfrom_rawsocket(socket_fd, 50, buffer, BUF_SIZE); /* timeout parcial 50 ms */
        
        if (bytes <= 0) {
            /* nada chegou nestes 50 ms */
            if (++quedas > 100) {               /* ≈5 s sem nada */
                puts("[CLIENTE] link ausente; reiniciando socket…");
                close(socket_fd);
                socket_fd = cria_raw_socket(interface);
                quedas = 0;
            }
            continue;
        }

        /* pacote chegou: zera contador de quedas */
        quedas = 0;

        if (!valid_kermit_pckt(pkt)) {              /* pacote corrompido   */
            responder_ack(NACK_TYPE, pkt->seq);     /* pede retransmissão  */
            continue;
        }

        /* --- respostas de movimentação --- */
        if (pkt->type == OKACK_TYPE) return 1;
        if (pkt->type == ACK_TYPE) return 0;
        if (pkt->type == NACK_TYPE) return -1;

        /* --- início de envio de tesouro --- */
        switch (pkt->type) {
            case TEXT_ACK_NAME:
            case IMG_ACK_NAME:
            case VIDEO_ACK_NAME:
                tipo_arquivo = pkt->type;
                memcpy(nome_arquivo, pkt->data, pkt->size);
                nome_arquivo[pkt->size] = '\0';
                aguardando_arquivo = 1;
                responder_ack(OKACK_TYPE, pkt->seq);   
                break;

            case TAM_TYPE:
                if (aguardando_arquivo && nome_arquivo[0]) {
                    memcpy(&tamanho_arquivo, pkt->data, sizeof(int));
                    responder_ack(OKACK_TYPE, pkt->seq);  /* ACK do tamanho */
                    if (arquivo_cabe(".", tamanho_arquivo)) receber_arquivo(tipo_arquivo, nome_arquivo, tamanho_arquivo);
                    else {
                        int enviou_erro = 0;
                        // envia pacote de erro - erro de tamanho
                        do {
                            byte_t erro = ERR_NO_SPACE;
                            kermit_pckt_t error_pkt;
                            gen_kermit_pckt(&error_pkt, seq++, ERROR_TYPE, &erro, 1);
                            enviou_erro = esperar_ack(&error_pkt);
                        } while (enviou_erro == 0);
                        printf("ERRO: Arquivo grande demais.");
                    }
                    /* limpa estado */
                    aguardando_arquivo = 0;
                    nome_arquivo[0] = '\0';
                    tipo_arquivo   = -1;
                    tamanho_arquivo= -1;
                    return 2;
                }
                break;

            case ERROR_TYPE:
                if (pkt->data[0] == (byte_t)ERR_NO_PERMISSION)
                    printf("ERRO: Servidor não teve permissão para acessar um dos arquivos.");
                else if (pkt->data[0] == (byte_t)ERR_COULD_NOT_OPEN)
                    printf("ERRO: Servidor falhou ao abrir um dos arquivos na hora do envio.");
                else
                    printf("ERRO: Servidor não encontrou o arquivo especificado do tesouro.");
                responder_ack(ACK_TYPE, pkt->seq);  
                return 2;

            default:
                break;
        }
    }
    /* tempo total (2000 ms) esgotado sem resposta útil */
    return -1;
}

void enviar_ping(int sig) {
    kermit_pckt_t ping;
    gen_kermit_pckt(&ping, 0, IDLE_TYPE, NULL, 0);
    sendto_rawsocket(socket_fd, &ping, sizeof(ping));
    alarm(4); // agenda próximo ping
}

int main(int argc, char *argv[]) {
    signal(SIGALRM, enviar_ping); // define a função que roda a cada alarme
    alarm(4); // agenda o primeiro alarme para daqui 3s
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <interface>\n", argv[0]);
        return EXIT_FAILURE;
    }

    interface = argv[1];

    socket_fd = cria_raw_socket(interface);
    printf("Cliente iniciado. Use W/A/S/D para mover. Q para sair.\n");

    while (1) {
        desenhar_mapa(posicao_jogador);

        char cmd = getchar();
        while (getchar() != '\n');
        if (cmd == 'q') break;

        int status = 0;
        do {
            if (cmd == 'w' || cmd == 'a' || cmd == 's' || cmd == 'd') {
                enviar_movimento(cmd);
                status = verificar_resposta();          /* 1 = OKACK, 0 = ACK, -1 = NACK/timeout */
                if (status == -1) printf("Reenviando comando…\n");
            } else printf("Comando inválido: %c\n", cmd); 
        } while (status == -1);

        if (mapa[posicao_jogador.y][posicao_jogador.x] != 1) mapa[posicao_jogador.y][posicao_jogador.x] = 2;
        if (status == 1 || status == 2) {
            if (cmd == 'w' && posicao_jogador.y < GRID_SIZE - 1) posicao_jogador.y++;
            if (cmd == 's' && posicao_jogador.y > 0)             posicao_jogador.y--;
            if (cmd == 'a' && posicao_jogador.x > 0)             posicao_jogador.x--;
            if (cmd == 'd' && posicao_jogador.x < GRID_SIZE - 1) posicao_jogador.x++;
        }
        if (status == 2) mapa[posicao_jogador.y][posicao_jogador.x] = 1;

        alarm(4);
    }

    close(socket_fd);
    return 0;
}