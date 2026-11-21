// ============================================================================
// GATEWAY — Dispenser de Medicamentos LoRa
// Placa: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
//
// Funcao: recebe os pacotes LoRa enviados pelo no dispenser e republica o
//         conteudo (JSON) em um broker MQTT, seguindo o contrato de dados
//         em doc/arquitetura/protocolo.md.
//
// Fluxo:  LoRa (recebe JSON)  ->  Wi-Fi  ->  MQTT publish em dispenser/<id>/evento
//
// Bibliotecas necessarias (Arduino IDE -> Gerenciador de Bibliotecas):
//   - RadioLib            (radio SX1262) — testado com a versao 8.0.0
//   - PubSubClient        (cliente MQTT)
//   - ArduinoJson         (ler o campo "id" do JSON para montar o topico)
//   - U8g2                (display OLED embutido)
//
// Antes de compilar: copie config.exemplo.h para config.h e preencha.
// ============================================================================

#include <RadioLib.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"

// ---- Pinos do SX1262 (FIXOS na Heltec V3 — ligacao interna, nao mexer) ----
#define LORA_NSS   8
#define LORA_DIO1  14
#define LORA_RST   12
#define LORA_BUSY  13
#define LORA_SCK   9
#define LORA_MISO  11
#define LORA_MOSI  10

// ---- Pinos do OLED embutido (FIXOS na Heltec V3) ----
#define OLED_SDA   17
#define OLED_SCL   18
#define OLED_RST   21
#define VEXT_CTRL  36   // controla a alimentacao do OLED (LOW = liga)

// ---- Parametros LoRa (DEVEM ser iguais nos dois lados: gateway e dispenser) ----
// RadioLib 8.x: frequencia e bandwidth sao inteiros COM UNIDADE (macros abaixo).
#define LORA_FREQ_MHZ    915     // MHz (banda livre no Brasil)
#define LORA_BW_KHZ      125     // kHz
#define LORA_SF          9       // spreading factor
#define LORA_CR          7       // coding rate 4/7 (valores validos: 5 a 8)
#define LORA_SYNC_WORD   0x34    // palavra de sincronismo (rede privada)
#define LORA_POWER       22      // dBm (maximo do SX1262)

// ---- Objetos ----
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

// Flag setada na interrupcao quando um pacote LoRa chega.
volatile bool pacoteRecebido = false;
void ISR_recepcao() { pacoteRecebido = true; }

// --------------------------------------------------------------------------
// Mostra duas linhas de texto no OLED.
// --------------------------------------------------------------------------
void mostrarOled(const String& linha1, const String& linha2) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 12, linha1.c_str());
  oled.drawStr(0, 28, linha2.c_str());
  oled.sendBuffer();
}

// --------------------------------------------------------------------------
// Conecta ao Wi-Fi (bloqueante, com feedback no OLED).
// --------------------------------------------------------------------------
void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(WIFI_SSID);
  mostrarOled("Wi-Fi...", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Wi-Fi conectado. IP: ");
  Serial.println(WiFi.localIP());
  mostrarOled("Wi-Fi OK", WiFi.localIP().toString());
}

// --------------------------------------------------------------------------
// (Re)conecta ao broker MQTT.
// --------------------------------------------------------------------------
void conectarMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando ao MQTT... ");
    mostrarOled("MQTT...", MQTT_HOST);
    bool ok;
    if (strlen(MQTT_USER) > 0) {
      ok = mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
    } else {
      ok = mqtt.connect(MQTT_CLIENT_ID);
    }
    if (ok) {
      Serial.println("conectado.");
      mostrarOled("MQTT OK", "Aguardando LoRa");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(mqtt.state());
      Serial.println(" — tentando em 2s");
      delay(2000);
    }
  }
}

// --------------------------------------------------------------------------
// Inicializa o radio LoRa (SX1262 da Heltec V3) em modo de recepcao continua.
// RadioLib 8.x: configuracao via struct ConfigLoRa_t; TCXO alimentado pelo
// DIO3 e RF switch no DIO2 (passos essenciais na Heltec V3).
// --------------------------------------------------------------------------
void iniciarLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  Serial.print("Iniciando LoRa... ");

  // RadioLib 8.x: configuracao via struct ConfigLoRa_t.
  // ATENCAO: frequency e bandwidth sao inteiros COM UNIDADE (macros abaixo).
  ConfigLoRa_t cfg;
  cfg.frequency       = RADIOLIB_UNIT_MEGA(LORA_FREQ_MHZ);  // 915 MHz
  cfg.bandwidth       = RADIOLIB_UNIT_KILO(LORA_BW_KHZ);    // 125 kHz
  cfg.spreadingFactor = LORA_SF;
  cfg.codingRate      = LORA_CR;
  cfg.syncWord        = LORA_SYNC_WORD;
  cfg.power           = LORA_POWER;

  int estado = radio.begin(cfg);
  if (estado != RADIOLIB_ERR_NONE) {
    Serial.print("FALHOU no begin(), codigo ");
    Serial.println(estado);
    mostrarOled("ERRO LoRa", "begin " + String(estado));
    while (true) { delay(1000); }   // trava: sem radio nao ha o que fazer
  }

  // ---- PASSO CRITICO NA HELTEC V3 ----
  // TCXO alimentado pelo DIO3 do SX1262 (RadioLib 8.x usa enum de tensao).
  estado = radio.setTCXO(Voltage1V8);
  if (estado != RADIOLIB_ERR_NONE) {
    Serial.print("Aviso: setTCXO() retornou ");
    Serial.println(estado);
  }
  // DIO2 controla o chaveamento da antena (RF switch) na Heltec V3.
  radio.setDio2AsRfSwitch(true);

  Serial.println("OK");

  // RadioLib 8.x: setIrqAction substitui setPacketReceivedAction.
  radio.setIrqAction(ISR_recepcao);
  radio.startReceive();
}

// --------------------------------------------------------------------------
// Extrai o campo "id" do JSON e publica o payload no topico correspondente.
// --------------------------------------------------------------------------
void publicarEvento(const String& payload) {
  // Descobre o id do dispenser para montar o topico dispenser/<id>/evento.
  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, payload);
  String id = "desconhecido";
  if (!erro && doc.containsKey("id")) {
    id = doc["id"].as<String>();
  }

  String topico = "dispenser/" + id + "/evento";
  bool ok = mqtt.publish(topico.c_str(), payload.c_str());

  Serial.print("Publicado em ");
  Serial.print(topico);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FALHA");

  String evt = (!erro && doc.containsKey("evt")) ? doc["evt"].as<String>() : "?";
  mostrarOled("Evento: " + evt, "de " + id);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // Liga a alimentacao do OLED (Vext) e inicializa o display.
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);
  delay(100);
  oled.begin();
  mostrarOled("Gateway", "Iniciando...");

  iniciarLoRa();
  conectarWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  conectarMQTT();

  Serial.println("Gateway pronto. Aguardando pacotes LoRa.");
}

void loop() {
  // Mantem a conexao MQTT viva.
  if (!mqtt.connected()) {
    conectarMQTT();
  }
  mqtt.loop();

  // Se chegou um pacote LoRa, le e publica.
  if (pacoteRecebido) {
    pacoteRecebido = false;

    String payload;
    int estado = radio.readData(payload);
    if (estado == RADIOLIB_ERR_NONE) {
      Serial.print("LoRa recebido: ");
      Serial.println(payload);
      Serial.print("  RSSI: ");
      Serial.print(radio.getRSSI());
      Serial.print(" dBm | SNR: ");
      Serial.print(radio.getSNR());
      Serial.println(" dB");

      publicarEvento(payload);
    } else {
      Serial.print("Erro ao ler pacote LoRa: ");
      Serial.println(estado);
    }

    radio.startReceive();   // volta a escutar
  }
}
