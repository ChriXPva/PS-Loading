#include "rede_mqtt.h"
#include "display_lcd.h"
#include "logger_serial.h"
#include "dados_sistema.h"
#include "estado_sistema.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID         = "Wokwi-GUEST";
const char* WIFI_PASSWORD     = "";
const int   WIFI_CHANNEL      = 6;

const char* MQTT_BROKER       = "broker.hivemq.com";
const int   MQTT_PORT         = 1883;

const char* TOPICO_TELEMETRIA = "loading/2026_2/embarcados/telemetria";
const char* TOPICO_HISTORICO  = "loading/2026_2/embarcados/historico";
const char* TOPICO_COMANDO    = "loading/2026_2/embarcados/comando";
const char* TOPICO_STATUS     = "loading/2026_2/embarcados/status";

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

void inicializarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
}

void callbackMqtt(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;

  if (doc["limite"].is<float>()) {
    float novoLim = doc["limite"].as<float>();
    dados_definirLimiteRemoto(novoLim);
    logSerialPrintf("[MQTT] Novo limite remoto definido: %.1f C\n", novoLim);
  }
  if (doc["modo_pot"].is<bool>() && doc["modo_pot"].as<bool>() == true) {
    dados_definirLimiteRemoto(-1.0f);
    logSerialPrintln("[MQTT] Controle de limite devolvido ao potenciometro.");
  }
}

void publicarStatusMqtt(const PacoteTelemetria &pacote) {
  if (!mqttClient.connected()) return;
  JsonDocument doc;

  doc["uptime_s"] = millis() / 1000;
  doc["wifi"]     = WiFi.status() == WL_CONNECTED ? "conectado" : "desconectado";
  doc["mqtt"]     = mqttClient.connected() ? "conectado" : "desconectado";
  doc["estado"]   = obterNomeEstado(pacote.estado);

  char bufferJson[256];
  size_t tamanho = serializeJson(doc, bufferJson);
  mqttClient.publish(TOPICO_STATUS, bufferJson, tamanho);
}

void publicarTelemetriaMqtt(const PacoteTelemetria &pacote) {
  if (!mqttClient.connected()) return;

  JsonDocument doc;
  doc["uptime_s"] = pacote.uptimeSeg;

  if (pacote.s1Ok) {
    doc["sensor1_c"] = serialized(String(pacote.tempS1, 1));
  } else {
    doc["sensor1_c"] = nullptr;
  }

  if (pacote.s2Ok) {
    doc["sensor2_c"] = serialized(String(pacote.tempS2, 1));
  } else {
    doc["sensor2_c"] = nullptr;
  }

  doc["limite_c"]  = serialized(String(pacote.limite, 1));
  doc["histerese"] = HISTERESE;
  doc["estado"]    = obterNomeEstado(pacote.estado);

  char bufferJson[256];
  serializeJson(doc, bufferJson);
  mqttClient.publish(TOPICO_TELEMETRIA, bufferJson);
}

void TaskRedeMQTT(void *pvParameters) {
  lcd_mostrarStatus("Conectando Wi-Fi", "");
  logSerialPrintf("[REDE] Conectando ao Wi-Fi ");

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(250));
    logSerialPrintf(".");
    lcd_mostrarProgresso(tentativas % 16, 1, ".");
    tentativas++;

    if (tentativas % 40 == 0) {
      logSerialPrintf("\n[REDE] Reiniciando tentativa Wi-Fi ");
      WiFi.disconnect(false);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
    }
  }

  logSerialPrintf("\n[REDE] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
  dados_setWifiConectado(true);
  lcd_mostrarStatus("Wi-Fi Conectado!", WiFi.localIP().toString().c_str());
  vTaskDelay(pdMS_TO_TICKS(1000));

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callbackMqtt);

  // Sinaliza que a rede está pronta para liberar as tarefas de controle e sensores
  xEventGroupSetBits(xEventosSistema, BIT_REDE_PRONTA);

  bool mqttProntoSinalizado = false;

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqttClient.connected()) {
        dados_setMqttConectado(false);
        String clientId = "ESP32-Loading-" + String(random(0xffff), HEX);
        if (mqttClient.connect(clientId.c_str())) {
          dados_setMqttConectado(true);
          mqttClient.subscribe(TOPICO_COMANDO);
          logSerialPrintln("[MQTT] Conectado ao Broker!");

          // Sinaliza exibição de leituras apenas na primeira conexão com o broker
          if (!mqttProntoSinalizado) {
            mqttProntoSinalizado = true;
            xEventGroupSetBits(xEventosSistema, BIT_MQTT_PRONTO);
          }
        } else {
          vTaskDelay(pdMS_TO_TICKS(2000));
          continue;
        }
      }

      mqttClient.loop();

      // Consome pacotes de telemetria da fila sem race condition
      PacoteTelemetria pacote;
      if (xFilaTelemetria && xQueueReceive(xFilaTelemetria, &pacote, pdMS_TO_TICKS(50)) == pdPASS) {
        publicarTelemetriaMqtt(pacote);
        publicarStatusMqtt(pacote);
      }
    } else {
      dados_setWifiConectado(false);
      dados_setMqttConectado(false);
      logSerialPrintln("[REDE] Conexao perdida, reconectando...");
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
      vTaskDelay(pdMS_TO_TICKS(3000));
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
