#ifndef __KERMIT_H__
#define __KERMIT_H__

#include "raw_socket.h"

#define BUF_SIZE (1024 + 1)  // Buffer auxiliar
#define DATA_SIZE (127)      // Tamanho máximo do campo de dados

#define TIMEOUT_MS (50)
#define TIMEOUT_PROB (980)  // Probabilidade de timeout (para simulações)
#define TIMEOUT_LIMIT (5)

typedef unsigned char byte_t;

// --- Códigos do protocolo ---

// Marcador e endereços
#define INIT_MARKER (0x7E)
#define CLI_ADDR (0x1)
#define SER_ADDR (0x2)

// Tipos de mensagem
#define ACK_TYPE         (0x0)
#define NACK_TYPE        (0x1)
#define OKACK_TYPE       (0x2)
#define LIVRE_TYPE       (0x3) // IDEIA : fazer ack que pede estado ao iniciar cliente (pede pro servidor mandar posição do jogador e tesouros ja encontrados)
#define TAM_TYPE         (0x4)
#define DATA_TYPE        (0x5)
#define TEXT_ACK_NAME    (0x6)
#define VIDEO_ACK_NAME   (0x7)
#define IMG_ACK_NAME     (0x8)
#define END_FILE_TYPE    (0x9)
#define MOVER_DIR        (0xA)
#define MOVER_CIMA       (0xB)
#define MOVER_BAIXO      (0xC)
#define MOVER_ESQ        (0xD)
#define LIVRE2_TYPE      (0xE) // IDEIA: fazer um que manda sem nada só pra avisar que ainda esta ON de tempo em tempo (ou em caso de inatividade do cliente)
#define ERROR_TYPE       (0xF)

// Códigos de erro
#define ERR_NO_PERMISSION   (0x0)
#define ERR_NO_SPACE        (0x1)

// Estrutura do pacote Kermit baseado na nova definição
typedef struct kermit_pckt_t
{
    byte_t init_marker;          // 8 bits: marcador de início (sempre 0x7E)
    byte_t size : 7;             // 7 bits: tamanho do campo de dados
    byte_t seq  : 5;             // 5 bits: número de sequência
    byte_t type : 4;             // 4 bits: tipo da mensagem
    byte_t checksum;             // 8 bits: paridade/checksum do pacote
    byte_t data[DATA_SIZE];      // 0 a 127 bytes: dados (conteúdo)
} kermit_pckt_t;

/*!
 * @brief Gera um pacote Kermit
 *
 * @param kpckt Ponteiro para o pacote a ser preenchido
 * @param seq Número de sequência do pacote
 * @param type Tipo da mensagem
 * @param data Ponteiro para os dados a serem inseridos
 * @param num_data Número de bytes de dados
 */
void gen_kermit_pckt(kermit_pckt_t *kpckt, int seq, int type,
                     void *data, size_t num_data);

/*!
 * @brief Imprime o conteúdo de um pacote
 *
 * @param kpckt Ponteiro para o pacote a ser impresso
 */
void print_kermit_pckt(kermit_pckt_t *kpckt);

/*!
 * @brief Verifica se o pacote é válido (paridade e formato)
 *
 * @param kpckt Ponteiro para o pacote a ser validado
 * @return 1 se válido, 0 se inválido
 */
int valid_kermit_pckt(kermit_pckt_t *kpckt);

/*!
 * @brief Detecta erros no pacote com base em paridade
 *
 * @param kpckt Ponteiro para o pacote a ser verificado
 * @return 1 se há erro, 0 se não há erro
 */
int error_detection(kermit_pckt_t *kpckt);

/*!
 * @brief Verifica se o pacote é um ACK
 *
 * @param pkt Ponteiro para o pacote
 * @return 1 se for ACK, 0 caso contrário
 */
int is_ack(kermit_pckt_t *pkt);

/*!
 * @brief Verifica se o pacote é um NACK
 *
 * @param pkt Ponteiro para o pacote
 * @return 1 se for NACK, 0 caso contrário
 */
int is_nack(kermit_pckt_t *pkt);

/*!
 * @brief Gera um pacote ACK
 *
 * @param ack Ponteiro para o pacote ACK a ser preenchido
 * @param seq Número de sequência correspondente
 */
void gen_ack(kermit_pckt_t *ack, byte_t seq);

/*!
 * @brief Gera um pacote NACK
 *
 * @param nack Ponteiro para o pacote NACK a ser preenchido
 * @param seq Número de sequência correspondente
 */
void gen_nack(kermit_pckt_t *nack, byte_t seq);

/*!
 * @brief Envia uma resposta do servidor ao cliente após tentativa de movimento.
 *
 * @param tipo Tipo de resposta: OKACK_TYPE (0x2), ACK_TYPE (0x0), NACK_TYPE (0x1)
 */
void responder_movimento(byte_t tipo);

#endif
