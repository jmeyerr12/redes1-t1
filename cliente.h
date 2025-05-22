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
 *        O arquivo pode ser texto (exibido no console) ou binário (salvo no disco).
 */
void receber_arquivo();

/*!
 * @brief Desenha o mapa 8x8 no terminal com a posição atual do jogador.
 *
 * @param jogador Estrutura contendo as coordenadas (x, y) do jogador.
 */
void desenhar_mapa(Posicao jogador)

#endif
