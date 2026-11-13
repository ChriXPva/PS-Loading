#ifndef HISTORICO_H
#define HISTORICO_H

#include "config.h"

// Inicialização do módulo histórico (cria mutex próprio)
void historico_inicializar();

// Gerenciamento e visualização do histórico de leituras em memória
void registrarHistorico(float b1, float b2, float m1, float m2, float lim, EstadoSistema est, unsigned long timestampSeg);
void imprimirHistoricoSerial(bool modoMedia = false);

#endif // HISTORICO_H
