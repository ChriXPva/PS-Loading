#include "sensor.h"
#include "filtro.h"
#include "estado_sistema.h"
#include "historico.h"
#include "dados_sistema.h"
#include "atuadores.h"
#include "logger_serial.h"
#include <atomic>

static DHT dht1(PIN_DHT_1, DHT_TYPE);
static DHT dht2(PIN_DHT_2, DHT_TYPE);

// Buffers de amostragem dos sensores
static float bufferS1[TAMANHO_JANELA_MEDIA];
static float bufferS2[TAMANHO_JANELA_MEDIA];
static int idxBufferS1 = 0, qtdAmostrasS1 = 0;
static int idxBufferS2 = 0, qtdAmostrasS2 = 0;

QueueHandle_t xFilaLog = NULL;
static std::atomic<uint32_t> s_pacotesPerdidos{0};

void sensores_inicializar() {
  dht1.begin();
  dht2.begin();
  xFilaLog = xQueueCreate(5, sizeof(PacoteLog));
}

bool validarLeitura(float temp) {
  return !isnan(temp) && (temp >= TEMP_MIN_VALIDA) && (temp <= TEMP_MAX_VALIDA);
}

float lerDHTSeguro(DHT &sensor) {
  float temp = NAN;
  for (int tentativa = 0; tentativa < 2; tentativa++) {
    temp = sensor.readTemperature();

    if (validarLeitura(temp)) {
      return temp;
    }
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  return temp;
}

// ============================================================================
// TASK FREERTOS DE SENSORES (Prioridade 3 - Aquisição em Tempo Real)
// ============================================================================
void TaskSensores(void *pvParameters) {
  xEventGroupWaitBits(xEventosSistema, BIT_REDE_PRONTA, pdFALSE, pdTRUE, portMAX_DELAY);

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequencia = pdMS_TO_TICKS(INTERVALO_AQUISICAO_MS);

  // Tolerância a falhas transitórias (máximo de 2 falhas consecutivas toleradas)
  static int falhasConsecutivasS1 = 0;
  static int falhasConsecutivasS2 = 0;
  static float ultimaValidaS1 = NAN;
  static float ultimaValidaS2 = NAN;

  for (;;) {
    // Capturar timestamp único no início do ciclo
    unsigned long agora = millis();
    unsigned long timestampSeg = agora / 1000;

    SnapshotDados snap;
    dados_obterSnapshot(snap);
    bool falhaManual = snap.falhaManual;

    float brutaS1 = falhaManual ? NAN : lerDHTSeguro(dht1);
    vTaskDelay(pdMS_TO_TICKS(20));
    float brutaS2 = lerDHTSeguro(dht2);

    bool s1Ok = validarLeitura(brutaS1);
    bool s2Ok = validarLeitura(brutaS2);

    // Contadores de falhas consecutivas e retenção da última leitura válida
    if (s1Ok) {
      falhasConsecutivasS1 = 0;
      ultimaValidaS1 = brutaS1;
    } else {
      falhasConsecutivasS1++;
    }

    if (s2Ok) {
      falhasConsecutivasS2 = 0;
      ultimaValidaS2 = brutaS2;
    } else {
      falhasConsecutivasS2++;
    }

    // Só entra em estado de falha se houver 3 ou mais falhas em sequência
    bool s1ValidoSistema = (falhasConsecutivasS1 < 3) && validarLeitura(ultimaValidaS1);
    bool s2ValidoSistema = (falhasConsecutivasS2 < 3) && validarLeitura(ultimaValidaS2);

    // Em falhas transitórias (1 ou 2 ciclos), mantém a última temperatura válida
    float tempEfetivaS1 = s1Ok ? brutaS1 : (s1ValidoSistema ? ultimaValidaS1 : NAN);
    float tempEfetivaS2 = s2Ok ? brutaS2 : (s2ValidoSistema ? ultimaValidaS2 : NAN);

    float filtradaS1 = s1ValidoSistema ? calcularMediaMovel(bufferS1, idxBufferS1, qtdAmostrasS1, tempEfetivaS1) : NAN;
    float filtradaS2 = s2ValidoSistema ? calcularMediaMovel(bufferS2, idxBufferS2, qtdAmostrasS2, tempEfetivaS2) : NAN;

    if (!s1ValidoSistema) limparFiltro(idxBufferS1, qtdAmostrasS1);
    if (!s2ValidoSistema) limparFiltro(idxBufferS2, qtdAmostrasS2);

    float limAtual = snap.limiteAtual;
    EstadoSistema estadoAnterior = snap.estadoAtual;
    bool sistemaIntegro = s1ValidoSistema && s2ValidoSistema;
    float tempCritica = sistemaIntegro ? max(tempEfetivaS1, tempEfetivaS2) : NAN;

    EstadoSistema novoEstado = processarEstado(tempCritica, limAtual, estadoAnterior, sistemaIntegro);

    // 1. Atualização em lote (uma única aquisição do mutex)
    dados_atualizarCiclo(tempEfetivaS1, tempEfetivaS2, s1ValidoSistema, s2ValidoSistema, novoEstado);

    // 2. Registra histórico com timestamp único
    registrarHistorico(tempEfetivaS1, tempEfetivaS2, filtradaS1, filtradaS2, limAtual, novoEstado, timestampSeg);

    // 3. Envia snapshot de telemetria via fila FreeRTOS para a rede
    if (xFilaTelemetria) {
      PacoteTelemetria pacote = {
        timestampSeg,
        tempEfetivaS1,
        tempEfetivaS2,
        s1ValidoSistema,
        s2ValidoSistema,
        limAtual,
        novoEstado
      };
      if (xQueueSend(xFilaTelemetria, &pacote, 0) != pdPASS) {
        s_pacotesPerdidos.fetch_add(1, std::memory_order_relaxed);
      }
    }

    // 4. Atualiza atuadores de hardware de forma imediata
    atualizarLeds(novoEstado);

    // 5. Enfileirar log sem bloquear
    if (xFilaLog) {
      PacoteLog pktLog = {
        timestampSeg,
        tempEfetivaS1,
        tempEfetivaS2,
        limAtual,
        novoEstado,
        s1ValidoSistema,
        s2ValidoSistema,
        s_pacotesPerdidos.exchange(0, std::memory_order_relaxed)
      };
      xQueueSend(xFilaLog, &pktLog, 0);
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequencia);
  }
}
