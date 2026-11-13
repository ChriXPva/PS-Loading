#ifndef DADOS_SISTEMA_H
#define DADOS_SISTEMA_H

#include "config.h"

// Repositório thread-safe para dados compartilhados do sistema
void dados_inicializar();

// Atualização e leitura de dados dos sensores
void dados_atualizarSensores(float t1, float t2, bool s1Ok, bool s2Ok, EstadoSistema est);
bool dados_obterLeituras(float &t1, float &t2, bool &s1Ok, bool &s2Ok, float &lim, EstadoSistema &est);

// Controle de limites
void dados_definirLimitePotenciometro(float limitePot);
void dados_definirLimiteRemoto(float limiteRemoto);
float dados_obterLimiteAtual();

// Falha simulada por botão
void dados_alternarFalhaManual();
bool dados_obterFalhaManual();

// Estado do sistema
EstadoSistema dados_obterEstado();

// Snapshot atômico para leitura consistente de múltiplos campos
struct SnapshotDados {
  bool falhaManual;
  float limiteAtual;
  EstadoSistema estadoAtual;
};

// Leitura consistente em uma única aquisição do mutex
void dados_obterSnapshot(SnapshotDados& snap);

// Atualização em lote dos dados do ciclo de sensores
void dados_atualizarCiclo(float t1, float t2, bool s1Ok, bool s2Ok, EstadoSistema est);

// Estado de conectividade
void dados_setMqttConectado(bool conectado);
bool dados_obterMqttConectado();
void dados_setWifiConectado(bool conectado);
bool dados_obterWifiConectado();

#endif // DADOS_SISTEMA_H
