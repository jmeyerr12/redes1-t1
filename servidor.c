#include "servidor.h"

Tesouro tesouros[MAX_TESOUROS];
int pos_x = 0, pos_y = 0;
int socket_fd;
char buffer[BUF_SIZE];
static int quedas = 0;

bool arquivo_existe(const char *caminho) {
    struct stat st;
    return (stat(caminho, &st) == 0);
}

bool tem_permissao_arquivo(const char *caminho) {
	struct stat st;
	if (stat(caminho, &st) != 0)
		return false;

	uid_t uid = getuid(); // uid real
	gid_t gid = getgid(); // gid real

	if (uid == st.st_uid)
		return (st.st_mode & S_IRUSR);

	if (gid == st.st_gid)
		return (st.st_mode & S_IRGRP);

	return (st.st_mode & S_IROTH);
}

void carregar_tesouros() {
    srand(time(NULL));

    int ocupado[GRID_SIZE][GRID_SIZE] = {0};
    ocupado[0][0] = 1; // evita (0,0)

    for (int i = 0; i < MAX_TESOUROS; i++) {
        int x, y;
        do {
            x = rand() % GRID_SIZE;
            y = rand() % GRID_SIZE;
        } while (ocupado[y][x]);

        ocupado[y][x] = 1;
        tesouros[i].x = x;
        tesouros[i].y = y;
        tesouros[i].encontrado = 0;

        char prefixo[3];
        snprintf(prefixo, sizeof(prefixo), "%d", i + 1);

        DIR *dir = opendir(OBJETOS_DIR);
        if (!dir) {
            perror("Erro ao abrir diretório de objetos");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG &&
                strncmp(entry->d_name, prefixo, strlen(prefixo)) == 0) {
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

void enviar_arquivo(const char *caminho, int seq) {
    if (!arquivo_existe(caminho)) {
        int enviou_erro = 0;
        //mandar erro pro cliente
        do {
            byte_t erro = ERR_DOESNT_EXIST;
            kermit_pckt_t error_pkt;
            gen_kermit_pckt(&error_pkt, seq++, ERROR_TYPE, &erro, 1);
            enviou_erro = esperar_ack(&error_pkt);
        } while (enviou_erro == 0);
        perror("ERRO: Arquivo especificado não existe");
        return;
    }

    if (!tem_permissao_arquivo(caminho)) {
        int enviou_erro = 0;
        //mandar erro pro cliente
        do {
            byte_t erro = ERR_NO_PERMISSION;
            kermit_pckt_t error_pkt;
            gen_kermit_pckt(&error_pkt, seq++, ERROR_TYPE, &erro, 1);
            enviou_erro = esperar_ack(&error_pkt);
        } while (enviou_erro == 0);
        perror("ERRO: Sem permissão para ler o tesouro");
        return;
    }
    
    FILE *fp = fopen(caminho, "rb");
    if (!fp) {
        int enviou_erro = 0;
        //mandar erro pro cliente
        do {
            byte_t erro = ERR_COULD_NOT_OPEN;
            kermit_pckt_t error_pkt;
            gen_kermit_pckt(&error_pkt, seq++, ERROR_TYPE, &erro, 1);
            enviou_erro = esperar_ack(&error_pkt);
        } while (enviou_erro == 0);
        perror("ERRO: Não foi possível abrir o tesouro");
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

    int status;

    // Nome
    do {
        gen_kermit_pckt(&pkt, seq, tipo, nome, strlen(nome));
        status = esperar_ack(&pkt); // 1 = OKACK, 0 = timeout/NACK
        if (status == 0) printf("Reenviando nome do arquivo…\n");
    } while (status == 0);
    seq++;

    // Tamanho
    do {
        gen_kermit_pckt(&pkt, seq, TAM_TYPE, &tamanho, sizeof(tamanho));
        status = esperar_ack(&pkt);
        if (status == 0) printf("Reenviando tamanho do arquivo…\n");
    } while (status == 0);
    seq++;

    // Dados
    size_t lidos;
    while ((lidos = fread(dados, 1, DATA_SIZE, fp)) > 0) {
        do {
            gen_kermit_pckt(&pkt, seq, DATA_TYPE, dados, lidos);
            status = esperar_ack(&pkt);
            if (status == 0) printf("Reenviando bloco de dados (seq = %d)…\n", seq%32);
        } while (status == 0);
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
        case MOVER_DIR: if (pos_x < GRID_SIZE - 1) pos_x++; else movimento_valido = 0; break;
        case MOVER_ESQ: if (pos_x > 0) pos_x--; else movimento_valido = 0; break;
        case MOVER_CIMA: if (pos_y < GRID_SIZE - 1) pos_y++; else movimento_valido = 0; break;
        case MOVER_BAIXO: if (pos_y > 0) pos_y--; else movimento_valido = 0; break;
        default:
            printf("Tipo de movimento inválido");
            responder_movimento(ACK_TYPE); // tipo de movimento inválido
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
        //print_kermit_pckt((kermit_pckt_t *) buffer);
        //kermit_pckt_t *pckt = (kermit_pckt_t *) buffer;
        //if (pckt->type == (0xE)) printf("%d\n", pckt->type);
        if (bytes <= 0) {
            /* nada chegou nestes 50 ms */
            if (++quedas > 100) {               /* ≈5 s sem nada */
                puts("[SERVIDOR] link ausente; reiniciando socket…");
                close(socket_fd);
                socket_fd = cria_raw_socket(argv[1]);
                quedas = 0;
            }
            continue;
        } else quedas = 0;

        kermit_pckt_t *pkt = (kermit_pckt_t *)buffer;
        if (!valid_kermit_pckt(pkt)) {
            printf("Pacote inválido recebido\n");
            responder_movimento(NACK_TYPE);
            continue;
        }

        if (pkt->type >= MOVER_DIR && pkt->type <= MOVER_ESQ) processar_movimento(pkt->type); //aqui ja manda nack
        else if (pkt->type == ERROR_TYPE){
            printf("ERRO: Cliente não conseguiu receber o arquivo enviado (tamanho muito grande).");
            responder_movimento(ACK_TYPE);
        }
    }

    return 0;
}
