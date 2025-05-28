#ifndef CLIENTE_H
#define CLIENTE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "raw_socket.h"
#include "kermit.h"

#define BUF_SIZE (1024 + 1)
#define GRID_SIZE 8

/*!
 * @brief Representa a posição atual do jogador no grid
 */
typedef struct {
    int x;
    int y;
} Posicao;

/*!
 * @brief Envia um comando de movimentação ao servidor
 *
 * @param comando Caractere representando a direção: 
 *        'w' (cima), 'a' (esquerda), 's' (baixo), 'd' (direita)
 */
void enviar_movimento(char comando);

/*!
 * @brief Recebe e processa um arquivo enviado pelo servidor.
 *
 * @param tipo Tipo do arquivo (TEXT_ACK_NAME, IMG_ACK_NAME ou VIDEO_ACK_NAME)
 * @param nome_arquivo Nome do arquivo recebido
 * @param tamanho Tamanho total do arquivo em bytes
 */
void receber_arquivo(int tipo, const char *nome_arquivo, int tamanho);

/*!
 * @brief Desenha o mapa 8x8 no terminal com a posição atual do jogador e com as posições dos tesouros encontrados.
 *
 * @param jogador Estrutura contendo as coordenadas (x, y) do jogador.
 */
void desenhar_mapa(Posicao jogador);

/*!
 * @brief Verifica se o servidor iniciou o envio de um arquivo e, se sim, processa-o.
 * 
 * @return -1 se receber NACK, 0 caso contrario
 */
int verificar_resposta();

void responder_ack(byte_t tipo, byte_t seq);

#endif
