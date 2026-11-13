#include "dados_sistema.h"

static struct {
  EstadoSistema estadoAtual;
  float limiteCalculado;
  float limiteRemoto;
  bool falhaManual;
  float tempS1;
  float tempS2;
  bool s1Valido;
  bool s2Valido;
  bool mqttConectado;
  bool wifiConectado;
} estadoInterno;

void dados_inicializar() {
  estadoInterno.estadoAtual = ESTADO_NORMAL;
  estadoInterno.limiteCalculado = 30.0f;
  estadoInterno.limiteRemoto = -1.0f;
  estadoInterno.falhaManual = false;
  estadoInterno.tempS1 = NAN;
  estadoInterno.tempS2 = NAN;
  estadoInterno.s1Valido = false;
  estadoInterno.s2Valido = false;
  estadoInterno.mqttConectado = false;
  estadoInterno.wifiConectado = false;
}

void dados_atualizarSensores(float t1, float t2, bool s1Ok, bool s2Ok, EstadoSistema est) {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    estadoInterno.tempS1 = t1;
    estadoInterno.tempS2 = t2;
    estadoInterno.s1Valido = s1Ok;
    estadoInterno.s2Valido = s2Ok;
    estadoInterno.estadoAtual = est;
    xSemaphoreGive(xMutexDados);
  }
}

bool dados_obterLeituras(float &t1, float &t2, bool &s1Ok, bool &s2Ok, float &lim, EstadoSistema &est) {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    t1 = estadoInterno.tempS1;
    t2 = estadoInterno.tempS2;
    s1Ok = estadoInterno.s1Valido;
    s2Ok = estadoInterno.s2Valido;
    lim = estadoInterno.limiteCalculado;
    est = estadoInterno.estadoAtual;
    xSemaphoreGive(xMutexDados);
    return true;
  }
  return false;
}

void dados_definirLimitePotenciometro(float limitePot) {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    if (estadoInterno.limiteRemoto < 0.0f) {
      estadoInterno.limiteCalculado = limitePot;
    }
    xSemaphoreGive(xMutexDados);
  }
}

void dados_definirLimiteRemoto(float limiteRemoto) {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    estadoInterno.limiteRemoto = limiteRemoto;
    if (limiteRemoto > 0.0f) {
      estadoInterno.limiteCalculado = limiteRemoto;
    }
    xSemaphoreGive(xMutexDados);
  }
}

float dados_obterLimiteAtual() {
  float lim = 30.0f;
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    lim = estadoInterno.limiteCalculado;
    xSemaphoreGive(xMutexDados);
  }
  return lim;
}

void dados_alternarFalhaManual() {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    estadoInterno.falhaManual = !estadoInterno.falhaManual;
    xSemaphoreGive(xMutexDados);
  }
}

bool dados_obterFalhaManual() {
  bool f = false;
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    f = estadoInterno.falhaManual;
    xSemaphoreGive(xMutexDados);
  }
  return f;
}

EstadoSistema dados_obterEstado() {
  EstadoSistema est = ESTADO_NORMAL;
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    est = estadoInterno.estadoAtual;
    xSemaphoreGive(xMutexDados);
  }
  return est;
}

void dados_obterSnapshot(SnapshotDados& snap) {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    snap.falhaManual   = estadoInterno.falhaManual;
    snap.limiteAtual   = estadoInterno.limiteCalculado;
    snap.estadoAtual   = estadoInterno.estadoAtual;
    xSemaphoreGive(xMutexDados);
  } else {
    // Fallback seguro se mutex não disponível
    snap.falhaManual = false;
    snap.limiteAtual = 30.0f;
    snap.estadoAtual = ESTADO_NORMAL;
  }
}

void dados_atualizarCiclo(float t1, float t2, bool s1Ok, bool s2Ok, EstadoSistema est) {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    estadoInterno.tempS1      = t1;
    estadoInterno.tempS2      = t2;
    estadoInterno.s1Valido    = s1Ok;
    estadoInterno.s2Valido    = s2Ok;
    estadoInterno.estadoAtual = est;
    xSemaphoreGive(xMutexDados);
  }
}

void dados_setMqttConectado(bool conectado) {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    estadoInterno.mqttConectado = conectado;
    xSemaphoreGive(xMutexDados);
  }
}

bool dados_obterMqttConectado() {
  bool v = false;
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    v = estadoInterno.mqttConectado;
    xSemaphoreGive(xMutexDados);
  }
  return v;
}

void dados_setWifiConectado(bool conectado) {
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    estadoInterno.wifiConectado = conectado;
    xSemaphoreGive(xMutexDados);
  }
}

bool dados_obterWifiConectado() {
  bool v = false;
  if (xSemaphoreTake(xMutexDados, pdMS_TO_TICKS(50)) == pdTRUE) {
    v = estadoInterno.wifiConectado;
    xSemaphoreGive(xMutexDados);
  }
  return v;
}
