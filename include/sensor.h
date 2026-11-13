#ifndef SENSOR_H
#define SENSOR_H

#include "config.h"
#include <DHT.h>
#include <freertos/queue.h>
#include "estado_sistema.h"

void sensores_inicializar();
bool validarLeitura(float temp);
float lerDHTSeguro(DHT &sensor);

struct PacoteLog {
  unsigned long timestampSeg;
  float t1, t2, lim;
  EstadoSistema estado;
  bool s1Ok, s2Ok;
  uint32_t pacotesPerdidos;
};

extern QueueHandle_t xFilaLog;

#endif // SENSOR_H
