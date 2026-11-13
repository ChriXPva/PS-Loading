#ifndef FILTRO_H
#define FILTRO_H

#include "config.h"

// Funções puras de filtragem matemática
float calcularMediaMovel(float buffer[], int &idx, int &qtd, float novaLeitura);
void limparFiltro(int &idx, int &qtd);

#endif // FILTRO_H
