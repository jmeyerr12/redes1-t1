#include "kermit.h"

/* Bytes especiais */
#define ESCAPE_BYTE  0x7D
#define ESCAPE_MASK  0x20          /* byte ^ 0x20  */
#define MARKER_BYTE  0x7E          /* init_marker  */

/* Tamanho máximo após escape: cada byte pode virar dois            */
#define ESCAPED_SIZE (2 * DATA_SIZE)

/* ------------------------------------------------------------------
 * Escapa dados binários para transmissão
 *   @src  : ponteiro para o bloco original
 *   @len  : quantidade de bytes em src
 *   @dest : buffer de destino (deve ter >= ESCAPED_SIZE)
 *
 * Retorna: quantidade de bytes gerados no buffer dest
 * ----------------------------------------------------------------- */
int escape_data(const byte_t *src, int len, byte_t *dest)
{
    int j = 0;

    for (int i = 0; i < len; i++) {
        if (src[i] == MARKER_BYTE || src[i] == ESCAPE_BYTE) {
            /* insere byte de escape e o byte alterado             */
            dest[j++] = ESCAPE_BYTE;
            dest[j++] = src[i] ^ ESCAPE_MASK;
        } else {
            dest[j++] = src[i];
        }
    }
    return j;     /* tamanho do bloco escapado */
}

/* ------------------------------------------------------------------
 * Desscapa dados binários
 *   @src  : ponteiro para o buffer recebido (possivelmente escapado)
 *   @len  : número de bytes em src
 *   @dest : buffer de destino para dados reais (sem escape)
 *
 * Retorna: quantidade de bytes válidos copiados para dest.
 * ----------------------------------------------------------------- */
int unescape_data(const byte_t *src, int len, byte_t *dest)
{
    int j = 0;

    for (int i = 0; i < len; i++) {
        if (src[i] == ESCAPE_BYTE && i + 1 < len) {
            /* byte 0x7D encontrado – pega o próximo e faz XOR 0x20 */
            dest[j++] = src[++i] ^ ESCAPE_MASK;
        } else {
            dest[j++] = src[i];
        }
    }
    return j;   /* tamanho real sem stuffing */
}


void gen_kermit_pckt(kermit_pckt_t *kp, int seq, int type,
                     const void *data, size_t len, int esc)
{
    byte_t tmp[ESCAPED_SIZE];
    const byte_t *src = (const byte_t *)data;

    int plen = 0;
    if (esc && data && len)
        plen = escape_data(src, len, tmp);          /* faz stuffing       */
    else if (data && len)
        plen = (int)len;                            /* copia cru          */

    memset(kp, 0, sizeof(*kp));
    kp->init_marker = INIT_MARKER;
    kp->size = (plen < DATA_SIZE) ? plen : DATA_SIZE;
    kp->seq  = seq;
    kp->type = type;

    if (plen)
        memcpy(kp->data, esc ? tmp : src, kp->size);

    /* checksum calculado sobre os bytes já escapados (se houver) */
    byte_t cs = kp->size ^ kp->seq ^ kp->type;
    for (int i = 0; i < kp->size; i++)
        cs ^= kp->data[i];
    kp->checksum = cs;
}

void print_kermit_pckt(kermit_pckt_t *kpckt) {
    printf("Packet:\n");
    printf("  Init marker: 0x%02X\n", kpckt->init_marker);
    printf("  Size: %u\n", kpckt->size);
    printf("  Seq: %u\n", kpckt->seq);
    printf("  Type: 0x%X\n", kpckt->type);
    printf("  Checksum: 0x%02X\n", kpckt->checksum);
    printf("  Data: ");
    for (int i = 0; i < kpckt->size; i++)
        printf("%02X ", kpckt->data[i]);
    printf("\n");
}

int valid_kermit_pckt(kermit_pckt_t *kpckt) {
    if (kpckt->init_marker != INIT_MARKER) return 0;
    return !error_detection(kpckt);
}

int error_detection(kermit_pckt_t *kpckt) {
    byte_t expected = kpckt->size ^ kpckt->seq ^ kpckt->type;
    for (int i = 0; i < kpckt->size; i++)
        expected ^= kpckt->data[i];

    return (expected != kpckt->checksum);
}

int is_ack(kermit_pckt_t *pkt) {
    return pkt->type == 0;
}

int is_nack(kermit_pckt_t *pkt) {
    return pkt->type == 1;
}

void gen_ack(kermit_pckt_t *ack, byte_t seq) {
    gen_kermit_pckt(ack, 0, seq, NULL, 0, 0); // tipo 0 = ACK, sem dados
}

void gen_nack(kermit_pckt_t *nack, byte_t seq) {
    gen_kermit_pckt(nack, 1, seq, NULL, 0, 0); // tipo 1 = NACK
}
