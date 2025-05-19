#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "kermit.h"

#define GRID_SIZE 8
#define NUM_TESOUROS 8
#define MAX_NOME_ARQ 64

typedef struct {
    int x;
    int y;
    char arquivo[MAX_NOME_ARQ];
    int encontrado;
} tesouro_t;

typedef struct {
    int x;
    int y;
} posicao_t;

void inicializar_tesouros(tesouro_t tesouros[NUM_TESOUROS]);
int verificar_tesouro(tesouro_t tesouros[NUM_TESOUROS], posicao_t pos, char *arquivo_encontrado);
void mostrar_log(posicao_t pos, const char *evento);

#endif