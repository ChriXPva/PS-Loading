#include "logger_serial.h"
#include "estado_sistema.h"
#include "auxi.h"
#include <stdarg.h>
#include <atomic>

static std::atomic<bool> s_modoHistorico{false};
static std::atomic<bool> s_usuarioDigitando{false};

void logger_inicializar() {
  s_modoHistorico.store(false, std::memory_order_relaxed);
  s_usuarioDigitando.store(false, std::memory_order_relaxed);
}

void logger_setModoHistorico(bool ativo) {
  s_modoHistorico.store(ativo, std::memory_order_release);
}

bool logger_isModoHistorico() {
  return s_modoHistorico.load(std::memory_order_acquire);
}

void logger_setUsuarioDigitando(bool digitando) {
  s_usuarioDigitando.store(digitando, std::memory_order_release);
}

bool logger_isUsuarioDigitando() {
  return s_usuarioDigitando.load(std::memory_order_acquire);
}

bool deveEmitirLogSerial() {
  return !s_modoHistorico.load(std::memory_order_acquire) && !s_usuarioDigitando.load(std::memory_order_acquire);
}

void logSerialPrintf(const char* formato, ...) {
  char buffer[256];
  va_list args;
  va_start(args, formato);
  vsnprintf(buffer, sizeof(buffer), formato, args);
  va_end(args);

  if (xMutexSerial && xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.print(buffer);
    xSemaphoreGive(xMutexSerial);
  } else {
    Serial.print(buffer);
  }
}

void logSerialPrintln(const char* msg) {
  if (xMutexSerial && xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.println(msg);
    xSemaphoreGive(xMutexSerial);
  } else {
    Serial.println(msg);
  }
}

void emitirLogSerial(float t1, float t2, float lim, EstadoSistema estado, bool s1Ok, bool s2Ok, unsigned long timestampSeg) {
  if (!deveEmitirLogSerial()) return;
  char bufS1[10], bufS2[10];
  formatarTemperatura(bufS1, sizeof(bufS1), t1, s1Ok);
  formatarTemperatura(bufS2, sizeof(bufS2), t2, s2Ok);
  logSerialPrintf("[%lus] S1: %s | S2: %s | Lim: %.1fC | Estado: %s\n",
                  timestampSeg, bufS1, bufS2, lim, obterNomeEstado(estado));
}
