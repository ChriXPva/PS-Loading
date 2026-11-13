#include "filtro.h"

float calcularMediaMovel(float buffer[], int &idx, int &qtd, float novaLeitura) {
  buffer[idx] = novaLeitura;
  idx = (idx + 1) % TAMANHO_JANELA_MEDIA;
  if (qtd < TAMANHO_JANELA_MEDIA) qtd++;

  float soma = 0.0;
  for (int i = 0; i < qtd; i++) {
    soma += buffer[i];
  }
  return soma / qtd;
}

void limparFiltro(int &idx, int &qtd) {
  idx = 0;
  qtd = 0;
}
