// ============================================================================
// config.exemplo.h — MODELO de configuração do GATEWAY
//
// COMO USAR:
//   1. Copie este arquivo para "config.h" (na mesma pasta).
//   2. Preencha com os dados reais da sua rede e do seu broker MQTT.
//   3. O arquivo "config.h" NÃO deve ser versionado (está no .gitignore),
//      para não expor senhas no repositório.
// ============================================================================

#ifndef CONFIG_H
#define CONFIG_H

// ---- Wi-Fi ----
#define WIFI_SSID      "NOME_DA_SUA_REDE"
#define WIFI_PASSWORD  "SENHA_DA_SUA_REDE"

// ---- Broker MQTT ----
// Pode ser um broker público para teste (ex.: "broker.hivemq.com", porta 1883)
// ou um Mosquitto local/na nuvem.
#define MQTT_HOST      "broker.hivemq.com"
#define MQTT_PORT      1883
#define MQTT_USER      ""      // deixe "" se o broker não exigir usuário
#define MQTT_PASSWORD  ""      // deixe "" se o broker não exigir senha
#define MQTT_CLIENT_ID "gateway-dispenser-01"

#endif  // CONFIG_H