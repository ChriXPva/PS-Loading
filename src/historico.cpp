#include "historico.h"
#include "estado_sistema.h"
#include "logger_serial.h"
#include "auxi.h"

static SemaphoreHandle_t xMutexHistorico = NULL;

static RegistroHistorico historico[TAMANHO_HISTORICO];
static int idxHistorico = 0;
static int totalHistorico = 0;

void historico_inicializar() {
  xMutexHistorico = xSemaphoreCreateMutex();
}

void registrarHistorico(float b1, float b2, float m1, float m2, float lim, EstadoSistema est, unsigned long timestampSeg) {
  if (xMutexHistorico && xSemaphoreTake(xMutexHistorico, pdMS_TO_TICKS(50)) == pdTRUE) {
    historico[idxHistorico] = { timestampSeg, b1, b2, m1, m2, lim, est };
    idxHistorico = (idxHistorico + 1) % TAMANHO_HISTORICO;
    if (totalHistorico < TAMANHO_HISTORICO) totalHistorico++;
    xSemaphoreGive(xMutexHistorico);
  }
}

void imprimirHistoricoSerial(bool modoMedia) {
  RegistroHistorico copia[TAMANHO_HISTORICO];
  int totalCopia = 0;
  int idxCopia = 0;

  // Cópia rápida sob mutex para evitar contenção com outras tarefas
  if (xMutexHistorico && xSemaphoreTake(xMutexHistorico, pdMS_TO_TICKS(100)) == pdTRUE) {
    memcpy(copia, historico, sizeof(historico));
    totalCopia = totalHistorico;
    idxCopia = idxHistorico;
    xSemaphoreGive(xMutexHistorico);
  } else {
    logSerialPrintln("\n[AVISO] Memoria ocupada, tente novamente em instantes.\n");
    return;
  }

  if (modoMedia) {
    logSerialPrintln("\n--- HISTORICO RECENTE DE LEITURAS (MEDIA MOVEL) ---");
  } else {
    logSerialPrintln("\n--- HISTORICO RECENTE DE LEITURAS (TEMPERATURA INSTANTANEA) ---");
  }

  if (totalCopia == 0) {
    logSerialPrintln("Nenhum registro gravado ainda.");
  } else {
    for (int i = 0; i < totalCopia; i++) {
      int pos = (idxCopia - totalCopia + i + TAMANHO_HISTORICO) % TAMANHO_HISTORICO;
      float t1 = modoMedia ? copia[pos].tempMediaS1 : copia[pos].tempBrutaS1;
      float t2 = modoMedia ? copia[pos].tempMediaS2 : copia[pos].tempBrutaS2;

      char s1Str[10], s2Str[10];
      formatarTemperatura(s1Str, sizeof(s1Str), t1, !isnan(t1));
      formatarTemperatura(s2Str, sizeof(s2Str), t2, !isnan(t2));

      logSerialPrintf("[%lus] S1: %s | S2: %s | Lim: %.1fC | Status: %s\n",
                      copia[pos].timestampSeg,
                      s1Str,
                      s2Str,
                      copia[pos].limite,
                      obterNomeEstado(copia[pos].estado));
    }
  }
  logSerialPrintln("---------------------------------------------------\n");
}
