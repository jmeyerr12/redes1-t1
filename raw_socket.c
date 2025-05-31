#include "raw_socket.h"

#define TIMEOUT_SEC 10  // Timeout de 2 segundos

int cria_raw_socket(char* nome_interface_rede) {
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }

    int ifindex = if_nametoindex(nome_interface_rede);

    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;

    if (bind(soquete, (struct sockaddr*)&endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }

    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;

    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: Verifique se a interface foi especificada corretamente.\n");
        exit(-1);
    }

    return soquete;
}

int sendto_rawsocket(int socket_fd, void *buf, size_t buf_size) {
    return send(socket_fd, buf, buf_size, 0);
}

long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000LL + tp.tv_usec / 1000;
}

int recvfrom_rawsocket(int soquete, int timeoutMillis, char* buffer, int tamanho_buffer) {
    long long comeco = timestamp();
    struct timeval timeout = { .tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000 };
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));

    int bytes_lidos;
    do {
        bytes_lidos = recv(soquete, buffer, tamanho_buffer, 0);
        if (bytes_lidos == -1) continue; // timeout ou erro

        kermit_pckt_t *pkt = (kermit_pckt_t *)buffer;

        if (valid_kermit_pckt(pkt)) {
            return bytes_lidos;
        } else {
            printf("Pacote inválido:\n");
            printf("  init_marker: 0x%02X\n", pkt->init_marker);
            printf("  size: %u\n", pkt->size);
            printf("  seq: %u\n", pkt->seq);
            printf("  type: 0x%X\n", pkt->type);
            printf("  checksum recebido: 0x%02X\n", pkt->checksum);

            // Calculando checksum esperado manualmente
            byte_t checksum_esperado = pkt->size ^ pkt->seq ^ pkt->type;
            for (int i = 0; i < pkt->size; i++)
                checksum_esperado ^= pkt->data[i];

            printf("  checksum esperado: 0x%02X\n", checksum_esperado);
            printf("  dados (hex):");
            for (int i = 0; i < pkt->size; i++) {
                printf(" %02X", pkt->data[i]);
            }
            printf("\n");

            return -1;
        }
    } while (timestamp() - comeco <= timeoutMillis);

    return -1;
}
