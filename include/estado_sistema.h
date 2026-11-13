#ifndef ESTADO_SISTEMA_H
#define ESTADO_SISTEMA_H

#include "config.h"

// Regras de negócio e transição de estados do sistema
const char* obterNomeEstado(EstadoSistema estado);
EstadoSistema processarEstado(float tempCritica, float limite, EstadoSistema atual, bool leituraValida);

#endif // ESTADO_SISTEMA_H
