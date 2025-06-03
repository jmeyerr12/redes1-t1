#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/*!
    @brief  Teste de alocação de memória

    @param  ptr Ponteiro para a região de memória alocada
*/
void allocation_test(void *ptr);

/*!
    @brief  Calcula o tempo corrente do programa

    @return Tempo corrente em milissegundos 
*/
double timestamp();

#endif

//debug
/* Packet:
  Init marker: 0x7E
  Size: 127
  Seq: 17
  Type: 0x5
  Checksum: 0xE1
  Data: 9E D3 E8 A5 E0 98 77 81 00 9C 53 7E E6 D1 BB ED 5E B5 35 81 1D 39 62 0F 08 E1 EC 90 F5 9A CA A9 BB 5D FA 06 53 55 5E CD CE 6F 23 B3 24 6C 77 AA F2 13 65 50 81 7F AD 81 93 D7 00 7F F3 CD 64 ED 06 7B 85 EE 23 99 71 4A 6A 16 55 C5 F1 EE F7 7E 56 39 99 DE 90 B9 DB 86 60 7A DB 23 CF 8F 71 C4 49 81 86 C7 D1 C3 62 2F E8 FA 4C 9F AC FB E4 DE 3D 7B 8D 43 BF D5 6A 61 A5 D0 74 AE 68 82 8A */