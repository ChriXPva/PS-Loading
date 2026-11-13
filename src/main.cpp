#include "config.h"
#include "dados_sistema.h"
#include "sensor.h"
#include "atuadores.h"
#include "display_lcd.h"
#include "logger_serial.h"
#include "rede_mqtt.h"
#include "historico.h"

// Instâncias globais dos primitivos de sincronização FreeRTOS
SemaphoreHandle_t xMutexDados     = NULL;
SemaphoreHandle_t xMutexLCD       = NULL;
SemaphoreHandle_t xMutexSerial    = NULL;
QueueHandle_t     xFilaTelemetria = NULL;
EventGroupHandle_t xEventosSistema = NULL;

void inicializarHardware() {
  dados_inicializar();
  historico_inicializar();  // NOVO — mutex próprio do histórico
  logger_inicializar();
  atuadores_inicializar();
  sensores_inicializar();
  lcd_inicializar();
}

void setup() {
  Serial.begin(115200);

  // 1. Criação dos primitivos de sincronização FreeRTOS
  xMutexDados     = xSemaphoreCreateMutex();
  xMutexLCD       = xSemaphoreCreateMutex();
  xMutexSerial    = xSemaphoreCreateMutex();
  xFilaTelemetria = xQueueCreate(5, sizeof(PacoteTelemetria));
  xEventosSistema = xEventGroupCreate();

  // 2. Inicialização dos subsistemas de hardware e modelo de dados
  inicializarHardware();
  inicializarWiFi();

  // 3. Criação das tarefas com priorização correta:
  // - TaskSensores (Prio 3, Core 1): Alta prioridade para garantir temporização rígida do DHT
  // - TaskRedeMQTT (Prio 2, Core 0): Prioridade média para manter a pilha de rede responsiva
  // - TaskControleIO (Prio 1, Core 1): Prioridade baixa para UI, CLI interativa e buzzer
  xTaskCreatePinnedToCore(TaskSensores,   "Sensores",      4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(TaskRedeMQTT,   "Rede_MQTT",     6144, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(TaskControleIO, "Controle_UI",   4096, NULL, 1, NULL, 1);

  logSerialPrintln("Comandos Serial: 'HIST TEMP', 'HIST MEDIA' e 'voltar'.");
}

void loop() {
  vTaskDelete(NULL);
}
