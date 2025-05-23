#include "servidor.h"

Tesouro tesouros[MAX_TESOUROS];
int pos_x = 0, pos_y = 0;
int socket_fd;
char buffer[BUF_SIZE];

void carregar_tesouros() {
    srand(time(NULL));

    for (int i = 0; i < MAX_TESOUROS; i++) {
        tesouros[i].x = rand() % GRID_SIZE;
        tesouros[i].y = rand() % GRID_SIZE;
        tesouros[i].encontrado = 0;

        // Monta o prefixo esperado (ex: "1", "2", ..., "8")
        char prefixo[3];
        snprintf(prefixo, sizeof(prefixo), "%d", i + 1);

        DIR *dir = opendir(OBJETOS_DIR);
        if (!dir) {
            perror("Erro ao abrir diretório de objetos");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Verifica se é um arquivo regular e se começa com o prefixo correto
            if (entry->d_type == DT_REG &&
                strncmp(entry->d_name, prefixo, strlen(prefixo)) == 0) {
                // Monta o caminho completo de forma segura
                snprintf(tesouros[i].nome_arquivo, sizeof(tesouros[i].nome_arquivo),
                         "%s/%.55s", OBJETOS_DIR, entry->d_name);
                break;
            }
        }

        closedir(dir);
    }
}

void print_tesouros() {
    printf("Tesouros sorteados:\n");
    for (int i = 0; i < MAX_TESOUROS; i++) {
        printf(" - [%d] (%d,%d) -> %s\n", i, tesouros[i].x, tesouros[i].y, tesouros[i].nome_arquivo);
    }
}

void enviar_arquivo(const char *caminho, int seq) {
    FILE *fp = fopen(caminho, "rb");
    if (!fp) {
        perror("Erro ao abrir tesouro");
        return;
    }

    struct stat st;
    stat(caminho, &st);
    size_t tamanho = st.st_size;

    char *ext = strrchr(caminho, '.');
    int tipo = TEXT_ACK_NAME;
    if (ext) {
        if (strcmp(ext, ".mp4") == 0) tipo = VIDEO_ACK_NAME;
        else if (strcmp(ext, ".jpg") == 0) tipo = IMG_ACK_NAME;
    }

    char nome[64];
    strncpy(nome, strrchr(caminho, '/') + 1, sizeof(nome) - 1);
    nome[sizeof(nome) - 1] = '\0';

    byte_t dados[DATA_SIZE];
    kermit_pckt_t pkt;
    char ack_buf[BUF_SIZE];
    kermit_pckt_t *resp = (kermit_pckt_t *)ack_buf;

    // 1. Nome
    gen_kermit_pckt(&pkt, seq, tipo, nome, strlen(nome));
    while (1) {
        sendto_rawsocket(socket_fd, &pkt, sizeof(pkt));
        int bytes = recvfrom_rawsocket(socket_fd, TIMEOUT_MS, ack_buf, BUF_SIZE);
        if (bytes > 0 && valid_kermit_pckt(resp) && resp->seq == pkt.seq) {
            if (resp->type == OKACK_TYPE) break;
        }
    }
    seq++;

    // 2. Tamanho
    gen_kermit_pckt(&pkt, seq, TAM_TYPE, &tamanho, sizeof(tamanho));
    while (1) {
        sendto_rawsocket(socket_fd, &pkt, sizeof(pkt));
        int bytes = recvfrom_rawsocket(socket_fd, TIMEOUT_MS, ack_buf, BUF_SIZE);
        if (bytes > 0 && valid_kermit_pckt(resp) && resp->seq == pkt.seq) {
            if (resp->type == OKACK_TYPE) break;
        }
    }
    seq++;

    // 3. Dados
    size_t lidos;
    while ((lidos = fread(dados, 1, DATA_SIZE, fp)) > 0) { //não ta recebendo o ack dos dados
        gen_kermit_pckt(&pkt, seq, DATA_TYPE, dados, lidos);
        while (1) {
            sendto_rawsocket(socket_fd, &pkt, sizeof(pkt));
            int bytes = recvfrom_rawsocket(socket_fd, TIMEOUT_MS, ack_buf, BUF_SIZE);
            printf("esperando okack dos dados");
            if (bytes > 0 && valid_kermit_pckt(resp) && resp->seq == pkt.seq) {
                if (resp->type == OKACK_TYPE) break;
            }
        }
        seq++;
    }

    // 4. Finalizador (sem ACK obrigatório)
    gen_kermit_pckt(&pkt, seq++, END_FILE_TYPE, NULL, 0);
    sendto_rawsocket(socket_fd, &pkt, sizeof(pkt));

    fclose(fp);
    printf("Arquivo '%s' enviado com sucesso.\n", caminho);
}

int verificar_tesouro() {
    for (int i = 0; i < MAX_TESOUROS; i++) {
        if (!tesouros[i].encontrado &&
            tesouros[i].x == pos_x && tesouros[i].y == pos_y) {
            tesouros[i].encontrado = 1;
            return i;
        }
    }
    return -1;
}

void responder_movimento(byte_t tipo) {
    kermit_pckt_t resposta;
    gen_kermit_pckt(&resposta, 0, tipo, NULL, 0);
    sendto_rawsocket(socket_fd, &resposta, sizeof(resposta));
}

void processar_movimento(byte_t tipo) {
    int movimento_valido = 1;

    switch (tipo) {
        case MOVER_DIR:  if (pos_x < GRID_SIZE - 1) pos_x++; else movimento_valido = 0; break;
        case MOVER_ESQ:  if (pos_x > 0) pos_x--; else movimento_valido = 0; break;
        case MOVER_CIMA: if (pos_y < GRID_SIZE - 1) pos_y++; else movimento_valido = 0; break;
        case MOVER_BAIXO:if (pos_y > 0) pos_y--; else movimento_valido = 0; break;
        default:
            responder_movimento(NACK_TYPE); // tipo de movimento inválido
            return;
    }

    if (!movimento_valido) {
        printf("Movimento inválido: fora do grid!\n");
        responder_movimento(ACK_TYPE); // ACK simples, mas sem movimentar
        return;
    }

    int id = verificar_tesouro();
    if (id != -1) {
        printf("Jogador moveu para: (%d, %d)\n", pos_x, pos_y);
        printf("Tesouro encontrado: %s\n", tesouros[id].nome_arquivo);
        enviar_arquivo(tesouros[id].nome_arquivo, 0);
    } else {
        printf("Jogador moveu para: (%d, %d)\n", pos_x, pos_y);
        responder_movimento(OKACK_TYPE); // movimento realizado com sucesso
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <interface>\n", argv[0]);
        return EXIT_FAILURE;
    }

    socket_fd = cria_raw_socket(argv[1]);
    carregar_tesouros();
    print_tesouros();

    while (1) {
        int bytes = recvfrom_rawsocket(socket_fd, TIMEOUT_MS, buffer, BUF_SIZE);
        if (bytes <= 0) continue;

        kermit_pckt_t *pkt = (kermit_pckt_t *)buffer;
        if (!valid_kermit_pckt(pkt)) {
            printf("Pacote inválido recebido\n");
            continue;
        }

        if (pkt->type >= MOVER_DIR && pkt->type <= MOVER_ESQ) {
            processar_movimento(pkt->type);
        }
    }

    return 0;
}
