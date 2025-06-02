#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include "raw_socket.h"
#include "kermit.h"

#define GRID_SIZE 8
#define MAX_TESOUROS 8
#define OBJETOS_DIR "objetos"

/*!
 * @brief Estrutura que representa um tesouro no mapa
 */
typedef struct {
    int x, y;                       ///< Coordenadas do tesouro
    int encontrado;                ///< Flag indicando se já foi encontrado
    char nome_arquivo[64];        ///< Nome do arquivo associado ao tesouro
} Tesouro;

/*!
 * @brief Estrutura que representa uma posicao no mapa
 */
typedef struct {
    int x;
    int y;
} Posicao;

/*!
 * @brief Carrega e sorteia as posições dos tesouros e seus arquivos
 */
void carregar_tesouros();

/*!
 * @brief Exibe no terminal todas as posições dos tesouros e os arquivos associados
 */
void print_tesouros();

/*!
 * @brief Envia um arquivo para o cliente, dividindo-o em pacotes Kermit
 *
 * @param caminho Caminho completo para o arquivo
 * @param seq Número de sequência inicial do pacote
 */
void enviar_arquivo(const char *caminho, int seq);

/*!
 * @brief Verifica se a posição atual do jogador contém um tesouro não descoberto
 *
 * @return Índice do tesouro encontrado ou -1 se não houver tesouro na posição
 */
int verificar_tesouro();

/*!
 * @brief Processa o movimento recebido do cliente, atualizando a posição do jogador
 *
 * @param tipo Tipo de mensagem (MOVER_DIR, MOVER_CIMA, etc.)
 */
void processar_movimento(byte_t tipo);

#endif
