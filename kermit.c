#include "kermit.h"

void gen_kermit_pckt(kermit_pckt_t *kpckt, int seq, int type, void *data, size_t num_data)
{
    memset(kpckt, 0, sizeof(*kpckt));

    kpckt->init_marker = INIT_MARKER;
    kpckt->size = (num_data < DATA_SIZE) ? num_data : DATA_SIZE;
    kpckt->seq = seq;
    kpckt->type = type;

    if (data && kpckt->size > 0)
        memcpy(kpckt->data, data, kpckt->size);

    // Calcula checksum: XOR de size, seq, type e data
    byte_t checksum = kpckt->size ^ kpckt->seq ^ kpckt->type;
    for (int i = 0; i < kpckt->size; i++)
        checksum ^= kpckt->data[i];

    kpckt->checksum = checksum;
}

void print_kermit_pckt(kermit_pckt_t *kpckt)
{
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

int valid_kermit_pckt(kermit_pckt_t *kpckt)
{
    return (kpckt->init_marker == INIT_MARKER);
}

int error_detection(kermit_pckt_t *kpckt)
{
    byte_t expected = kpckt->size ^ kpckt->seq ^ kpckt->type;
    for (int i = 0; i < kpckt->size; i++)
        expected ^= kpckt->data[i];

    return (expected != kpckt->checksum);
}
