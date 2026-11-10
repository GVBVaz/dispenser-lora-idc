# Guia de Teste Completo — do Zero

Guia passo a passo para testar todo o firmware do projeto partindo do zero, tendo
apenas os arquivos do repositório e os componentes do laboratório. Cobre desde a
instalação dos programas até a validação da cadeia completa (dispenser → LoRa →
gateway → MQTT).

> Leia junto com [`README.md`](./README.md) (tabela de ligações e solução de problemas).

## Checklist de materiais

- [ ] 2x placa Heltec WiFi LoRa 32 V3 (com as 2 antenas LoRa)
- [ ] 2x cabo USB-C (dado, não só carga)
- [ ] 1x sensor infravermelho TCRT5000 (módulo com saída digital D0)
- [ ] 1x RTC DS1302 (com bateria)
- [ ] 1x buzzer ativo
- [ ] 1x LED + 1x resistor 220 Ω
- [ ] protoboard + jumpers (macho-macho e macho-fêmea)
- [ ] 1 computador com acesso à internet
- [ ] uma rede Wi-Fi 2,4 GHz (pode ser roteador do celular)

> ⚠ A Heltec V3 usa **ESP32-S3** e Wi-Fi **2,4 GHz** (não funciona em 5 GHz).

---

# PARTE 1 — Preparar o computador (uma vez)

## 1.1. Instalar o Arduino IDE

1. Baixe o Arduino IDE 2.x em https://www.arduino.cc/en/software e instale.
2. Abra o programa.

## 1.2. Adicionar suporte à placa Heltec V3

1. **Arquivo → Preferências**.
2. Em **"URLs Adicionais para Gerenciadores de Placas"**, cole:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **OK**.
4. **Ferramentas → Placa → Gerenciador de Placas** → procure **esp32** (Espressif) → **Instalar**.
   (Pode demorar — baixa bastante coisa.)

## 1.3. Instalar as bibliotecas

**Ferramentas → Gerenciar Bibliotecas** e instale (procure pelo nome exato):

- [ ] **RadioLib**
- [ ] **ArduinoJson**
- [ ] **U8g2**
- [ ] **Rtc by Makuna**
- [ ] **PubSubClient**

## 1.4. Driver USB (se a placa não aparecer)

Se ao conectar a placa não surgir nenhuma porta COM/tty, instale o driver **CP210x**
(Silicon Labs) ou **CH34x** (conforme o conversor USB da placa). No Linux, pode ser
preciso adicionar seu usuário ao grupo `dialout`:
```bash
sudo usermod -a -G dialout $USER   # faça logout/login depois
```

---

# PARTE 2 — Gravar o firmware pela primeira vez (teste de fumaça)

Objetivo: garantir que você consegue compilar e gravar nas placas, antes de montar
qualquer circuito.

## 2.1. Abrir os sketches

- Abra `firmware/gateway/gateway.ino` no Arduino IDE.
- Numa segunda janela, abra `firmware/dispenser/dispenser.ino`.

## 2.2. Criar o config.h do gateway

1. Na pasta `firmware/gateway/`, copie `config.exemplo.h` para um novo arquivo
   chamado **`config.h`** (mesmo diretório).
2. Abra `config.h` e preencha:
   - `WIFI_SSID` e `WIFI_PASSWORD` — sua rede 2,4 GHz.
   - `MQTT_HOST` — deixe `broker.hivemq.com` (broker público, para teste).
   - `MQTT_PORT` — `1883`.
   - `MQTT_USER` e `MQTT_PASSWORD` — deixe `""` (o HiveMQ público não exige).
3. Salve.

> O `config.h` não vai para o Git (está protegido no `.gitignore`), pois contém a
> senha do Wi-Fi.

## 2.3. Configurar a placa no IDE

Em **Ferramentas**:
- **Placa:** Heltec WiFi LoRa 32(V3) *(ou "ESP32S3 Dev Module" se não houver)*
- **USB CDC On Boot:** **Enabled** ← essencial para ver o Monitor Serial
- **Upload Speed:** 921600
- **Porta:** a que aparecer ao conectar a placa

## 2.4. Gravar

> **Conecte a antena LoRa ANTES de ligar a placa.** Transmitir sem antena danifica o rádio.

1. Conecte a **placa A** (será o gateway) no USB.
2. Com o `gateway.ino` aberto, clique em **Upload** (seta →).
3. Se der erro de upload, segure o botão **BOOT** / **PRG** da placa durante a gravação,
   ou baixe o Upload Speed para 115200.
4. Repita com a **placa B** e o `dispenser.ino`.

✔ Se as duas gravaram sem erro, a base está pronta.

---

# PARTE 3 — Teste incremental (a ordem importa)

Teste **uma etapa de cada vez**. Só avance quando a anterior passar.

## ETAPA A — LoRa entre as duas placas (sem nenhum componente ligado)

Objetivo: provar que as duas placas conversam por LoRa. **Não depende de nuvem,
banco ou sensores.**

1. Ligue a **placa A (gateway)** no USB e abra o **Monitor Serial** (ícone de lupa no
   canto superior direito), velocidade **115200**.
2. Observe as mensagens: deve mostrar `LoRa ... OK`, `Wi-Fi conectado`, `MQTT ... conectado`.
   - Se travar em "Wi-Fi": confira SSID/senha e que a rede é 2,4 GHz.
   - Se travar em "MQTT": confira que `MQTT_HOST` é `broker.hivemq.com` e a porta 1883.
3. Ligue a **placa B (dispenser)** no USB (pode ser na mesma máquina, em outra janela
   do IDE, ou em outro computador/power bank). A cada boot ela envia um `heartbeat`.
4. **No Monitor Serial do gateway** deve aparecer algo como:
   ```
   LoRa recebido: {"id":"disp-01","evt":"heartbeat","ts":"...","comp":1,"fw":"1.0.0"}
     RSSI: -42 dBm | SNR: 10 dB
   ```
5. Para gerar mais eventos, aperte o botão **RST** do dispenser (reinicia e manda outro heartbeat).

✔ **PASSOU** quando o gateway mostra o JSON recebido. Isso já valida a comunicação
LoRa (item obrigatório).

❌ Se não recebe nada: confira as antenas, e que os dois sketches têm os mesmos
parâmetros LoRa (frequência, SF, BW, sync word — já vêm iguais, não altere só um lado).

## ETAPA B — Sensor infravermelho TCRT5000 (no dispenser)

Objetivo: detectar presença/ausência do remédio.

1. **Desligue** a placa do dispenser antes de ligar fios.
2. Monte o TCRT5000 (ver `README.md` seção 2.1):
   - VCC → **3V3** | GND → **GND** | **D0 → GPIO 6**
3. Religue e abra o Monitor Serial.
4. Posicione o "remédio" (qualquer objeto) na frente do sensor e **ajuste o
   potenciômetro** do módulo até o LED indicador do próprio módulo acender.
5. **Remova** o objeto: deve aparecer no Serial `Retirada detectada!` e um evento ser enviado.
6. Se a lógica estiver invertida (acusa retirada com o objeto presente), troque no
   `dispenser.ino`:  `#define SENSOR_ATIVO_EM_LOW true` → `false`, e regrave.

✔ **PASSOU** quando "remover o objeto" gera `Retirada detectada!`.

## ETAPA C — RTC DS1302 (no dispenser)

Objetivo: ter data/hora confiável nos eventos.

1. Desligue a placa. Ligue o DS1302 (ver seção 2.1):
   - VCC → **3V3** | GND → **GND** | CLK → **GPIO 4** | DAT → **GPIO 3** | RST → **GPIO 2**
2. Religue e grave o `dispenser.ino` (na 1ª vez ele acerta a hora pela hora de compilação).
3. Confira no Serial que o campo `ts` dos eventos tem data/hora coerentes.
4. **Desligue e religue** a placa: a hora deve se manter.

✔ **PASSOU** quando o timestamp está correto e persiste após desligar.
(Se a hora zera, troque a bateria do DS1302 e regrave.)

## ETAPA D — Alerta e lógica de dose (no dispenser)

Objetivo: alerta no horário + evento de atraso.

1. Desligue a placa. Ligue:
   - Buzzer (+) → **GPIO 7**, (−) → GND
   - LED ânodo → **GPIO 45** passando por resistor 220 Ω; cátodo → GND
2. No `dispenser.ino`, para testar rápido, ajuste:
   - `DOSE_HORA`/`DOSE_MINUTO` para **1–2 minutos à frente** da hora atual do RTC.
   - `TOLERANCIA_MIN` para **1** (minuto).
3. Regrave e aguarde o horário: buzzer + LED acionam e sai o evento `alerta_disparado`.
4. **Não** remova o remédio e aguarde 1 min: deve sair o evento `atraso`.
5. ✔ Depois do teste, **reponha** os valores reais (ex.: dose 08:00, tolerância 30).

✔ **PASSOU** quando o alerta dispara no horário e o atraso é gerado após a tolerância.

## ETAPA E — Cadeia completa até o MQTT (dispenser → gateway → nuvem)

Objetivo: ver os eventos chegando na internet (MQTT). **Ainda sem Node-RED/banco** —
usamos um broker público e um visualizador.

Escolha UMA das formas de "assinar" o tópico:

**Opção 1 — Pelo navegador (não instala nada):**
1. Acesse o HiveMQ Web Client: http://www.hivemq.com/demos/websocket-client/
2. Em **Connection**, use Host `broker.hivemq.com`, porta `8000` (WebSocket) → **Connect**.
3. Em **Subscriptions → Add New Topic Subscription**, tópico: `dispenser/+/evento` → **Subscribe**.

**Opção 2 — Pelo terminal (se tiver o mosquitto):**
```bash
# instalar (uma vez): Ubuntu/Debian
sudo apt install mosquitto-clients
# assinar:
mosquitto_sub -h broker.hivemq.com -t 'dispenser/+/evento' -v
```

Depois:
1. Garanta que o **gateway** está ligado e conectado (Etapa A ok).
2. Gere eventos no **dispenser** (remova o objeto do sensor, ou reinicie para heartbeat).
3. As mensagens JSON devem aparecer no navegador/terminal, no tópico
   `dispenser/disp-01/evento`.

✔ **PASSOU** quando as mensagens do dispenser aparecem no assinante MQTT. A partir
daqui, as frentes de **Node-RED** e **banco** só precisam assinar o mesmo tópico.

---

# PARTE 4 — Registrar as evidências (para a apresentação)

Para a apresentação (vale 3 pts) é exigido mostrar evidências de funcionamento.
Durante os testes, capture:

- [ ] Foto/vídeo das duas placas conversando (OLED mostrando status).
- [ ] Print do Monitor Serial do gateway recebendo o JSON (Etapa A).
- [ ] Vídeo da retirada acionando o evento (Etapa B).
- [ ] Print do RTC com data/hora correta (Etapa C).
- [ ] Vídeo do buzzer/LED no horário da dose (Etapa D).
- [ ] Print do HiveMQ/mosquitto recebendo as mensagens (Etapa E).

---

# Resumo: o que dá para testar sozinho (sem os colegas)

| Etapa | Depende de banco/Node-RED? | Como verificar |
|---|---|---|
| A — LoRa | Não | Monitor Serial do gateway |
| B — Sensor | Não | Monitor Serial do dispenser |
| C — RTC | Não | Monitor Serial / OLED |
| D — Alerta | Não | buzzer + LED + Serial |
| E — MQTT | Não (broker público) | HiveMQ Web Client ou mosquitto_sub |

Todas as etapas são testáveis de forma independente. As frentes de Node-RED e banco
de dados apenas se conectam ao mesmo broker MQTT mais tarde, sem exigir mudança no
firmware.