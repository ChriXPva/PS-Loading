#ifndef DISPLAY_LCD_H
#define DISPLAY_LCD_H

#include "config.h"

// Inicialização e controle thread-safe do display LCD I2C
void lcd_inicializar();
void lcd_mostrarStatus(const char* linha1, const char* linha2);
void lcd_mostrarProgresso(int coluna, int linha, const char* caractere);
void lcd_atualizarTelemetria(float t1, float t2, float lim, EstadoSistema estado, bool s1Ok, bool s2Ok, bool mqttOk);

#endif // DISPLAY_LCD_H
