#ifndef LOGGER_SERIAL_H
#define LOGGER_SERIAL_H

#include "config.h"

// Inicialização do logger
void logger_inicializar();

// Controle de contexto de UI serial
void logger_setModoHistorico(bool ativo);
bool logger_isModoHistorico();

void logger_setUsuarioDigitando(bool digitando);
bool logger_isUsuarioDigitando();

bool deveEmitirLogSerial();

// Emissão segura e sincronizada na Serial
void logSerialPrintf(const char* formato, ...);
void logSerialPrintln(const char* msg);
void emitirLogSerial(float t1, float t2, float lim, EstadoSistema estado, bool s1Ok, bool s2Ok, unsigned long timestampSeg);

#endif // LOGGER_SERIAL_H
