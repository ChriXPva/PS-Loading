#include "estado_sistema.h"

const char* obterNomeEstado(EstadoSistema estado) {
  switch (estado) {
    case ESTADO_NORMAL: return "NORMAL";
    case ESTADO_ALERTA: return "ALERTA";
    case ESTADO_FALHA:  return "FALHA";
    default:            return "ERRO";
  }
}

EstadoSistema processarEstado(float tempCritica, float limite, EstadoSistema atual, bool leituraValida) {
  if (!leituraValida) return ESTADO_FALHA;

  if (atual == ESTADO_ALERTA) {
    return (tempCritica < (limite - HISTERESE)) ? ESTADO_NORMAL : ESTADO_ALERTA;
  } else {
    return (tempCritica >= limite) ? ESTADO_ALERTA : ESTADO_NORMAL;
  }
}
