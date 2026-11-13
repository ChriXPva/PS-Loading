#include "atuadores.h"

void atuadores_inicializar() {
  pinMode(PIN_LED_NORMAL, OUTPUT);
  pinMode(PIN_LED_ALERTA, OUTPUT);
  pinMode(PIN_LED_FALHA, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_FALHA, INPUT_PULLUP);
}

void atualizarLeds(EstadoSistema estado) {
  digitalWrite(PIN_LED_NORMAL, estado == ESTADO_NORMAL ? HIGH : LOW);
  digitalWrite(PIN_LED_ALERTA, estado == ESTADO_ALERTA ? HIGH : LOW);
  digitalWrite(PIN_LED_FALHA,  estado == ESTADO_FALHA  ? HIGH : LOW);
}
