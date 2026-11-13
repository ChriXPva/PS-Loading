#include "display_lcd.h"
#include "estado_sistema.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

static LiquidCrystal_I2C lcd(0x27, 16, 2);

void lcd_inicializar() {
  Wire.begin(I2C_SDA, I2C_SCL);
  if (xMutexLCD && xSemaphoreTake(xMutexLCD, pdMS_TO_TICKS(200)) == pdTRUE) {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Monitor Pro v3.0");
    lcd.setCursor(0, 1);
    lcd.print("Iniciando...");
    xSemaphoreGive(xMutexLCD);
  }
}

void lcd_mostrarStatus(const char* linha1, const char* linha2) {
  if (xMutexLCD && xSemaphoreTake(xMutexLCD, pdMS_TO_TICKS(100)) == pdTRUE) {
    lcd.clear();
    if (linha1) {
      lcd.setCursor(0, 0);
      lcd.print(linha1);
    }
    if (linha2) {
      lcd.setCursor(0, 1);
      lcd.print(linha2);
    }
    xSemaphoreGive(xMutexLCD);
  }
}

void lcd_mostrarProgresso(int coluna, int linha, const char* caractere) {
  if (xMutexLCD && xSemaphoreTake(xMutexLCD, pdMS_TO_TICKS(100)) == pdTRUE) {
    lcd.setCursor(coluna % 16, linha);
    lcd.print(caractere);
    xSemaphoreGive(xMutexLCD);
  }
}

void lcd_atualizarTelemetria(float t1, float t2, float lim, EstadoSistema estado, bool s1Ok, bool s2Ok, bool mqttOk) {
  if (xMutexLCD && xSemaphoreTake(xMutexLCD, pdMS_TO_TICKS(100)) == pdTRUE) {
    char bufS1[8], bufS2[8];
    if (s1Ok) { snprintf(bufS1, sizeof(bufS1), "%.1f", t1); } else { snprintf(bufS1, sizeof(bufS1), "ERR"); }
    if (s2Ok) { snprintf(bufS2, sizeof(bufS2), "%.1f", t2); } else { snprintf(bufS2, sizeof(bufS2), "ERR"); }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("S1:");
    lcd.print(bufS1);
    lcd.print(" S2:");
    lcd.print(bufS2);

    lcd.setCursor(0, 1);
    char linha2[17];
    char estadoStr[7];
    strncpy(estadoStr, obterNomeEstado(estado), 6);
    estadoStr[6] = '\0';
    snprintf(linha2, sizeof(linha2), "L:%.0fC %-6s %s", lim, estadoStr, mqttOk ? "  " : "NC");
    lcd.print(linha2);
    xSemaphoreGive(xMutexLCD);
  }
}
