#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

// PINAGEM (ESP32-S3)

#define PIN_DHT_1       10
#define PIN_DHT_2       11
#define DHT_TYPE        DHT22

#define PIN_LED_NORMAL  2
#define PIN_LED_ALERTA  6
#define PIN_LED_FALHA   5
#define PIN_BUZZER      17

#define PIN_BTN_FALHA   18
#define PIN_POT_LIMITE  4

#define I2C_SDA         8
#define I2C_SCL         9

// PARÂMETROS DO SISTEMA

#define INTERVALO_AQUISICAO_MS 2500
#define INTERVALO_DISPLAY_MS   2000
#define TEMP_MIN_VALIDA        -40.0f
#define TEMP_MAX_VALIDA        80.0f
#define HISTERESE              1.0f
#define TAMANHO_JANELA_MEDIA   5
#define TAMANHO_HISTORICO      10

#define BIT_REDE_PRONTA        (1 << 0)
#define BIT_MQTT_PRONTO        (1 << 1)

// TIPOS BÁSICOS

enum EstadoSistema {
  ESTADO_NORMAL,
  ESTADO_ALERTA,
  ESTADO_FALHA
};

struct RegistroHistorico {
  unsigned long timestampSeg;
  float tempBrutaS1;
  float tempBrutaS2;
  float tempMediaS1;
  float tempMediaS2;
  float limite;
  EstadoSistema estado;
};

struct PacoteTelemetria {
  unsigned long uptimeSeg;
  float tempS1;
  float tempS2;
  bool s1Ok;
  bool s2Ok;
  float limite;
  EstadoSistema estado;
};

// SINCRONIZAÇÃO FREERTOS (EXTERN)

extern SemaphoreHandle_t xMutexDados;
extern SemaphoreHandle_t xMutexLCD;
extern SemaphoreHandle_t xMutexSerial;
extern QueueHandle_t     xFilaTelemetria;
extern EventGroupHandle_t xEventosSistema;

// PROTÓTIPOS DAS TASKS FREERTOS

void TaskControleIO(void *pvParameters);
void TaskSensores(void *pvParameters);
void TaskRedeMQTT(void *pvParameters);

#endif // CONFIG_H
