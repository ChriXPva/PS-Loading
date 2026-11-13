#ifndef REDE_MQTT_H
#define REDE_MQTT_H

#include "config.h"

void inicializarWiFi();
void callbackMqtt(char* topic, byte* payload, unsigned int length);
void publicarTelemetriaMqtt(const PacoteTelemetria &pacote);

#endif // REDE_MQTT_H
