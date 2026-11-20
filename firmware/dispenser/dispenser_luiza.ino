// ============================================================================
// DISPENSER — Dispenser de Medicamentos LoRa
// Placa: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
//
// Funcao: detecta a retirada do medicamento (sensor infravermelho TCRT5000),
//         controla os horarios das doses (RTC DS1302), emite alerta (buzzer +
//         LED) e transmite os eventos via LoRa para o gateway, no formato
//         definido em doc/arquitetura/protocolo.md.
//
// Eventos enviados (campo "evt"): retirada | alerta_disparado | atraso | heartbeat
//
// Bibliotecas necessarias (Arduino IDE -> Gerenciador de Bibliotecas):
//   - RadioLib    (radio SX1262 — testado com a versao 8.0.0)
//   - ArduinoJson (montar o JSON do evento — versao 7.x)
//   - Rtc by Makuna (RTC DS1302 — procure por "Rtc by Makuna")
//   - U8g2        (display OLED embutido)
//
// Sensor: componente TCRT5000 CRU (nao modulo) — infravermelho reflexivo.
// LED IR em serie com 220 ohm (3.3V); fototransistor com pull-up de 10k para
// 3.3V e emissor no GND; o ponto de leitura vai para um pino ANALOGICO (ADC).
// ============================================================================

#include <RadioLib.h>
#include <ArduinoJson.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <U8g2lib.h>
#include <Wire.h>

// ============================================================================
// CONFIGURACAO — ajuste conforme o seu cenario
// ============================================================================
#define DISPOSITIVO_ID    "disp-01"   // id deste dispenser (ver contrato de dados)
#define FIRMWARE_VERSAO   "1.0.0"
#define COMPARTIMENTO      1          // numero do compartimento do prototipo

// Horario da dose a ser monitorada (formato 24h). No prototipo, uma dose.
// TODO: no futuro isso pode vir da configuracao via BLE.
#define DOSE_HORA          8          // 08
#define DOSE_MINUTO        0          // 08:00
#define TOLERANCIA_MIN     30         // janela de tolerancia (minutos) antes de marcar atraso

// ============================================================================
// ACERTO DO RELOGIO (RTC DS1302) — fazer UMA vez, com a bateria instalada
// ----------------------------------------------------------------------------
// O DS1302 nao "descobre" a hora sozinho: alguem precisa acerta-lo uma vez, e
// a bateria CR2032 mantem a contagem com a placa desligada.
//
// COMO ACERTAR A HORA REAL:
//   1. Coloque a bateria CR2032 no modulo.
//   2. Descomente a linha ACERTAR_RELOGIO abaixo e ponha a data/hora REAL
//      (ano, mes, dia, hora, minuto, segundo) — horario de Brasilia.
//   3. Compile e grave. O relogio sera acertado com esse valor.
//   4. Comente a linha de novo (coloque // na frente) e grave outra vez.
//      Isso e ESSENCIAL: se deixar descomentada, a cada reinicio o relogio
//      volta para esse horario fixo, em vez de continuar contando.
//
// Com a linha comentada, o firmware so acerta o relogio automaticamente (pela
// hora de compilacao) quando a hora estiver invalida (ex.: sem bateria).
// ----------------------------------------------------------------------------
 #define ACERTAR_RELOGIO   RtcDateTime(2026, 11, 18, 07, 56, 0)
// ============================================================================

// Sensor TCRT5000 CRU (componente, nao modulo): leitura ANALOGICA.
// Nesta montagem (coletor no ponto de leitura + pull-up de 10k para 3.3V e
// emissor no GND), a tensao no GPIO CAI quando ha objeto refletindo perto.
// Portanto "ha medicamento" = leitura ABAIXO do limiar.
//
// LIMIAR_IR e o valor de corte do analogRead (0..4095 no ESP32-S3). Ajuste-o
// observando os valores crus impressos no Monitor Serial (ver medicamentoPresente).
#define LIMIAR_IR            2000

// Se a deteccao ficar invertida (acusar retirada com o remedio presente, ou
// vice-versa), troque para "false" — inverte a comparacao com o limiar.
#define OBJETO_ABAIXO_DO_LIMIAR  false

// Intervalo do heartbeat (sinal de vida) em milissegundos.
#define HEARTBEAT_MS       (5UL * 60UL * 1000UL)   // 5 minutos

// ---- Pinos dos perifericos (GPIOs livres na Heltec V3) ----
#define PIN_SENSOR_IR 5    // TCRT5000 (coletor do fototransistor) — pino ANALOGICO (ADC1)
#define PIN_RTC_CLK   4    // DS1302 CLK (SCLK)
#define PIN_RTC_DAT   3    // DS1302 DAT (I/O)
#define PIN_RTC_RST   2    // DS1302 RST (CE)
#define PIN_BUZZER    7    // buzzer ativo
#define PIN_LED       45   // LED de alerta

// ---- Pinos do SX1262 (FIXOS na Heltec V3) ----
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
#define VEXT_CTRL  36

// ---- Parametros LoRa (DEVEM ser iguais aos do gateway) ----
// RadioLib 8.x: frequencia e bandwidth sao inteiros COM UNIDADE (macros abaixo).
#define LORA_FREQ_MHZ    915
#define LORA_BW_KHZ      125
#define LORA_SF          9
#define LORA_CR          7       // coding rate 4/7 (valores validos: 5 a 8)
#define LORA_SYNC_WORD   0x34
#define LORA_POWER       22

// ============================================================================
// Objetos
// ============================================================================
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);
ThreeWire fiosRtc(PIN_RTC_DAT, PIN_RTC_CLK, PIN_RTC_RST);  // ordem: IO, SCLK, CE
RtcDS1302<ThreeWire> rtc(fiosRtc);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

// ---- Estado ----
bool medicamentoAntes = true;        // estado anterior do compartimento
bool alertaDisparadoHoje = false;    // evita repetir o alerta da mesma dose
bool atrasoRegistradoHoje = false;   // evita repetir o evento de atraso
int  ultimoDiaProcessado = -1;       // para resetar as flags a cada novo dia
unsigned long ultimoHeartbeat = 0;

// --------------------------------------------------------------------------
// OLED: mostra duas linhas.
// --------------------------------------------------------------------------
void mostrarOled(const String& l1, const String& l2) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 12, l1.c_str());
  oled.drawStr(0, 28, l2.c_str());
  oled.sendBuffer();
}

// --------------------------------------------------------------------------
// Le o sensor IR (TCRT5000 cru) por leitura ANALOGICA e informa se HA
// medicamento no compartimento. A tensao e convertida pelo pull-up de 10k;
// com objeto refletindo perto a leitura cai abaixo do limiar (ver #defines).
// --------------------------------------------------------------------------
bool medicamentoPresente() {
  int leitura = analogRead(PIN_SENSOR_IR);

  // Imprime o valor cru para calibrar o LIMIAR_IR no laboratorio.
  Serial.print("IR analogico: ");
  Serial.println(leitura);

  bool detectouObjeto = OBJETO_ABAIXO_DO_LIMIAR ? (leitura < LIMIAR_IR)
                                                : (leitura > LIMIAR_IR);
  return detectouObjeto;
}

// --------------------------------------------------------------------------
// Monta o timestamp ISO-8601 (YYYY-MM-DDTHH:MM:SS) a partir do RTC.
// --------------------------------------------------------------------------
String timestampAgora() {
  RtcDateTime agora = rtc.GetDateTime();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04u-%02u-%02uT%02u:%02u:%02u",
           agora.Year(), agora.Month(), agora.Day(),
           agora.Hour(), agora.Minute(), agora.Second());
  return String(buf);
}

// --------------------------------------------------------------------------
// Monta o JSON do evento (conforme o contrato) e transmite por LoRa.
// --------------------------------------------------------------------------
void enviarEvento(const char* tipoEvento) {
  JsonDocument doc;
  doc["id"]   = DISPOSITIVO_ID;
  doc["evt"]  = tipoEvento;
  doc["ts"]   = timestampAgora();
  doc["comp"] = COMPARTIMENTO;
  doc["fw"]   = FIRMWARE_VERSAO;

  String payload;
  serializeJson(doc, payload);

  Serial.print("Enviando LoRa: ");
  Serial.println(payload);

  int estado = radio.transmit(payload);
  if (estado == RADIOLIB_ERR_NONE) {
    Serial.println("  -> enviado com sucesso");
  } else {
    Serial.print("  -> falha no envio, codigo ");
    Serial.println(estado);
  }
}

// --------------------------------------------------------------------------
// Aciona o alerta (buzzer + LED) por alguns segundos.
// --------------------------------------------------------------------------
void acionarAlerta() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(PIN_LED, HIGH);
    delay(300);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED, LOW);
    delay(300);
  }
}

// --------------------------------------------------------------------------
// Inicializa o radio LoRa (SX1262 da Heltec V3).
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
    while (true) { delay(1000); }
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
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SENSOR_IR, INPUT);
  delay(300);

  // PIN_SENSOR_IR e lido via analogRead (ADC) — nao precisa de pinMode.
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, LOW);

  // Liga o OLED.
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);
  delay(100);
  oled.begin();
  mostrarOled("Dispenser", "Iniciando...");

  // Inicializa o RTC DS1302.
  rtc.Begin();
  // O DS1302 precisa estar com escrita habilitada e rodando ANTES de acertar.
  if (rtc.GetIsWriteProtected()) rtc.SetIsWriteProtected(false);
  if (!rtc.GetIsRunning())       rtc.SetIsRunning(true);

#ifdef ACERTAR_RELOGIO
  // Modo de acerto manual: grava a data/hora REAL definida no #define do topo.
  // Lembre de comentar o #define e regravar depois, senao o relogio reseta
  // para este horario a cada boot.
  Serial.println("ACERTANDO o relogio com a data/hora manual definida no codigo.");
  rtc.SetDateTime(ACERTAR_RELOGIO);
#else
  if (!rtc.IsDateTimeValid()) {
    // Hora invalida (primeira vez ou sem bateria no RTC): ajusta pela
    // data/hora da compilacao. Para a hora REAL, use o #define ACERTAR_RELOGIO.
    Serial.println("RTC sem hora valida — ajustando pela hora de compilacao.");
    rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  }
#endif

  iniciarLoRa();

  // Estado inicial do compartimento.
  medicamentoAntes = medicamentoPresente();

  mostrarOled("Dispenser OK", "Monitorando");
  enviarEvento("heartbeat");          // avisa que esta online
  ultimoHeartbeat = millis();
}

void loop() {
  RtcDateTime agora = rtc.GetDateTime();

  // Reseta as flags diarias quando o dia muda.
  if (agora.Day() != ultimoDiaProcessado) {
    ultimoDiaProcessado = agora.Day();
    alertaDisparadoHoje = false;
    atrasoRegistradoHoje = false;
  }

  // ---- 1) Detecta retirada do medicamento ----
  bool presenteAgora = medicamentoPresente();
  if (medicamentoAntes && !presenteAgora) {
    // Transicao presente -> ausente = retirada.
    Serial.println("Retirada detectada!");
    mostrarOled("Retirada!", timestampAgora());
    enviarEvento("retirada");
  }
  medicamentoAntes = presenteAgora;

  // ---- 2) Alerta no horario da dose ----
  if (!alertaDisparadoHoje &&
      agora.Hour() == DOSE_HORA && agora.Minute() == DOSE_MINUTO) {
    Serial.println("Horario da dose — acionando alerta.");
    mostrarOled("Hora da dose!", "Tome o remedio");
    enviarEvento("alerta_disparado");
    acionarAlerta();
    alertaDisparadoHoje = true;
  }

  // ---- 3) Atraso: passou a janela de tolerancia e o remedio ainda esta la ----
  if (alertaDisparadoHoje && !atrasoRegistradoHoje && presenteAgora) {
    int minutosDesdeDose =
        (agora.Hour() * 60 + agora.Minute()) - (DOSE_HORA * 60 + DOSE_MINUTO);
    if (minutosDesdeDose >= TOLERANCIA_MIN) {
      Serial.println("Dose em atraso!");
      mostrarOled("ATRASO", "Dose nao tomada");
      enviarEvento("atraso");
      atrasoRegistradoHoje = true;
    }
  }

  // ---- 4) Heartbeat periodico ----
  if (millis() - ultimoHeartbeat >= HEARTBEAT_MS) {
    enviarEvento("heartbeat");
    ultimoHeartbeat = millis();
  }

  delay(200);   // ritmo de amostragem do sensor
}
