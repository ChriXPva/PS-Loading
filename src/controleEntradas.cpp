#include "config.h"
#include "dados_sistema.h"
#include "historico.h"
#include "logger_serial.h"
#include "display_lcd.h"
#include "sensor.h"

static void processarComando(const char* cmdStr) {
  String cmd = String(cmdStr);
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.equalsIgnoreCase("HIST MEDIA") || cmd.equalsIgnoreCase("HIST_MEDIA")) {
    logger_setModoHistorico(true);
    imprimirHistoricoSerial(true);
    logSerialPrintln("[INFO] Visualizacao de historico (MEDIA MOVEL) ativa.");
    logSerialPrintln("[INFO] Leituras continuam no display LCD e gravadas em memoria.");
    logSerialPrintln("[INFO] Digite 'voltar' para retornar a exibicao no monitor serial.\n");
  } else if (cmd.equalsIgnoreCase("HIST TEMP") || cmd.equalsIgnoreCase("HIST_TEMP")) {
    logger_setModoHistorico(true);
    imprimirHistoricoSerial(false);
    logSerialPrintln("[INFO] Visualizacao de historico (TEMP INSTANTANEA) ativa.");
    logSerialPrintln("[INFO] Leituras continuam no display LCD e gravadas em memoria.");
    logSerialPrintln("[INFO] Digite 'voltar' para retornar a exibicao no monitor serial.\n");
  } else if (cmd.equalsIgnoreCase("VOLTAR")) {
    if (logger_isModoHistorico()) {
      logger_setModoHistorico(false);
      logSerialPrintln("\n[INFO] Saindo do modo de historico. Exibicao das leituras retomada no monitor serial.\n");
    } else {
      logSerialPrintln("\n[ERRO] Comando 'voltar' usado de forma errada! O historico nao esta sendo visualizado.");
      logSerialPrintln("[DICA] Digite 'HIST TEMP' ou 'HIST MEDIA' antes de usar 'voltar'.\n");
    }
  } else {
    logSerialPrintf("\n[COMANDO] Comando '%s' desconhecido. Use 'HIST TEMP', 'HIST MEDIA' ou 'voltar'.\n\n", cmd.c_str());
  }
}

// ============================================================================
// TASK FREERTOS DE CONTROLE E UI (Prioridade 1, Core 1 - Background Interativo)
// ============================================================================
void TaskControleIO(void *pvParameters) {
  // Lê o potenciômetro imediatamente ao subir, antes de qualquer barreira,
  // para que o limite inicial reflita a posição real do pot e não o valor
  // padrão de 30 °C — evita falso alerta na primeira leitura dos sensores.
  {
    int adcInicial = analogRead(PIN_POT_LIMITE);
    float limiteInicial = 15.0f + (adcInicial / 4095.0f) * 35.0f;
    dados_definirLimitePotenciometro(limiteInicial);
  }

  // Aguarda WiFi conectado (BIT_REDE_PRONTA) e primeiro broker conectado (BIT_MQTT_PRONTO)
  xEventGroupWaitBits(xEventosSistema, BIT_REDE_PRONTA | BIT_MQTT_PRONTO, pdFALSE, pdTRUE, portMAX_DELAY);

  bool ultimoBtn = HIGH;
  unsigned long ultimoDebounce = 0;
  int ultimoAdcPot = -1;
  unsigned long ultimoBipeAlarme = 0;
  unsigned long ultimoUpdateLCD = 0;
  bool estadoBuzzer = false;

  static char bufferCmd[64];
  static size_t idxCmd = 0;
  static unsigned long ultimaDigitacao = 0;

  for (;;) {
    unsigned long agora = millis();

    // 1. Leitura caractere a caractere não-bloqueante com eco na tela
    while (Serial.available() > 0) {
      char c = (char)Serial.read();
      ultimaDigitacao = agora;

      if (c == '\r' || c == '\n') {
        if (idxCmd > 0) {
          logSerialPrintln(""); // Pula linha após o texto digitado
          bufferCmd[idxCmd] = '\0';
          processarComando(bufferCmd);
          idxCmd = 0;
          logger_setUsuarioDigitando(false);
        }
      } else if (c == '\b' || c == 127) {
        if (idxCmd > 0) {
          idxCmd--;
          logSerialPrintf("\b \b");
        }
        logger_setUsuarioDigitando(idxCmd > 0);
      } else if (c >= 32 && c <= 126) {
        if (idxCmd < sizeof(bufferCmd) - 1) {
          bufferCmd[idxCmd++] = c;
          logSerialPrintf("%c", c);
          logger_setUsuarioDigitando(true);
        }
      }
    }

    // Cancela digitação órfã após 20 segundos de inatividade
    if (idxCmd > 0 && (agora - ultimaDigitacao > 20000)) {
      logSerialPrintln("\n[AVISO] Tempo de digitacao esgotado. Comando cancelado.");
      idxCmd = 0;
      logger_setUsuarioDigitando(false);
    }

    // 2. Leitura do botão de falha simulada (com debounce)
    bool btnAtual = digitalRead(PIN_BTN_FALHA);
    if (ultimoBtn == HIGH && btnAtual == LOW && (agora - ultimoDebounce > 200)) {
      ultimoDebounce = agora;
      dados_alternarFalhaManual();
      bool falhaAtiva = dados_obterFalhaManual();
      logSerialPrintf("[DEBUG] Falha simulada S1: %s\n", falhaAtiva ? "ATIVA" : "INATIVA");
    }
    ultimoBtn = btnAtual;

    // 3. Leitura do potenciômetro e controle de limite
    int adcAtual = analogRead(PIN_POT_LIMITE);
    if (ultimoAdcPot != -1 && abs(adcAtual - ultimoAdcPot) > 120) {
      dados_definirLimiteRemoto(-1.0f); // Devolve o controle ao potenciômetro
    }
    ultimoAdcPot = adcAtual;

    float limitePot = 15.0f + (adcAtual / 4095.0f) * 35.0f;
    dados_definirLimitePotenciometro(limitePot);

    // 4. Controle do alarme sonoro (Buzzer)
    EstadoSistema est = dados_obterEstado();
    if (est == ESTADO_NORMAL) {
      if (estadoBuzzer) {
        noTone(PIN_BUZZER);
        estadoBuzzer = false;
      }
    } else {
      unsigned long intervaloBipe = (est == ESTADO_FALHA) ? 300 : 800;
      int frequencia = (est == ESTADO_FALHA) ? 1000 : 2000;

      if (agora - ultimoBipeAlarme >= intervaloBipe) {
        ultimoBipeAlarme = agora;
        estadoBuzzer = !estadoBuzzer;
        if (estadoBuzzer) {
          tone(PIN_BUZZER, frequencia);
        } else {
          noTone(PIN_BUZZER);
        }
      }
    }

    // 5. Atualização periódica e sincronizada: display LCD + log serial juntos
    if (agora - ultimoUpdateLCD >= INTERVALO_DISPLAY_MS) {
      ultimoUpdateLCD = agora;
      float t1, t2, lim;
      bool s1Ok, s2Ok;
      EstadoSistema estadoLido;
      if (dados_obterLeituras(t1, t2, s1Ok, s2Ok, lim, estadoLido)) {
        bool mqttOk = dados_obterMqttConectado();
        lcd_atualizarTelemetria(t1, t2, lim, estadoLido, s1Ok, s2Ok, mqttOk);

        // Log serial sincronizado com o LCD: drena apenas o pacote mais recente da fila
        if (xFilaLog) {
          PacoteLog pktLog;
          PacoteLog ultimoPkt;
          bool temPacote = false;
          // Descarta pacotes intermediários, mantém só o mais recente
          while (xQueueReceive(xFilaLog, &pktLog, 0) == pdPASS) {
            ultimoPkt = pktLog;
            temPacote = true;
          }
          if (temPacote) {
            emitirLogSerial(ultimoPkt.t1, ultimoPkt.t2, ultimoPkt.lim, ultimoPkt.estado,
                            ultimoPkt.s1Ok, ultimoPkt.s2Ok, ultimoPkt.timestampSeg);
            if (ultimoPkt.pacotesPerdidos > 0) {
              logSerialPrintf("[WARN] %lu pacotes de telemetria descartados por overflow.\n",
                              (unsigned long)ultimoPkt.pacotesPerdidos);
            }
          }
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
