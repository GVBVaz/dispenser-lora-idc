# Firmware — Dispenser de Medicamentos LoRa

Firmware dos dois nós do sistema, para a placa **Heltec WiFi LoRa 32 V3**
(ESP32-S3 + SX1262), desenvolvido no **Arduino IDE**.

- `dispenser/dispenser.ino` — nó que fica junto ao remédio: lê o sensor, controla
  o horário das doses, emite alerta e **transmite** os eventos por LoRa.
- `gateway/gateway.ino` — nó concentrador: **recebe** os eventos por LoRa e os
  **publica** em um broker MQTT.

> Ambos seguem o contrato de dados em [`../doc/arquitetura/protocolo.md`](../doc/arquitetura/protocolo.md).

---

## 1. Preparar o Arduino IDE (fazer uma vez)

### 1.1. Instalar o suporte à placa Heltec V3 (ESP32-S3)

1. Abra **Arquivo → Preferências**.
2. No campo **"URLs Adicionais para Gerenciadores de Placas"**, adicione:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Abra **Ferramentas → Placa → Gerenciador de Placas**, procure por **esp32**
   (da Espressif) e instale.
4. Em **Ferramentas → Placa → ESP32**, selecione **Heltec WiFi LoRa 32(V3)**.
   (Se não aparecer com esse nome exato, use **ESP32S3 Dev Module**.)

### 1.2. Instalar as bibliotecas

Em **Ferramentas → Gerenciar Bibliotecas**, instale:

| Biblioteca | Para quê | Usada em |
|---|---|---|
| **RadioLib** | rádio LoRa SX1262 | dispenser + gateway |
| **ArduinoJson** | montar/ler o JSON do evento | dispenser + gateway |
| **U8g2** | display OLED embutido | dispenser + gateway |
| **Rtc by Makuna** | RTC DS1302 | dispenser |
| **PubSubClient** | cliente MQTT | gateway |

> O sensor infravermelho TCRT5000 não requer biblioteca — é lido por `digitalRead`.

### 1.3. Configuração ao gravar (Ferramentas)

- **Placa:** Heltec WiFi LoRa 32(V3)
- **Upload Speed:** 921600 (se der erro de upload, baixe para 115200)
- **USB CDC On Boot:** Enabled *(importante para ver o Monitor Serial)*
- **Porta:** a COM/tty que aparecer ao conectar a placa via USB

---

## 2. Ligações de hardware (apenas no nó DISPENSER)

O **gateway não precisa de ligação nenhuma** — ele só usa o rádio e o OLED, que já
são internos da Heltec V3. Basta alimentá-lo por USB.

No **dispenser**, ligamos o sensor infravermelho, o RTC e o alerta. O rádio e o OLED
já são internos (não precisam de fios).

### 2.1. Tabela de ligações

| Componente | Pino do componente | GPIO na Heltec V3 | Observação |
|---|---|---|---|
| TCRT5000 | VCC | 3V3 | alimente em 3,3V (saída fica em nível seguro) |
| TCRT5000 | GND | GND | — |
| TCRT5000 | D0 (saída digital) | GPIO **6** | ligação direta (sem divisor) |
| DS1302 | VCC | 3V3 | — |
| DS1302 | GND | GND | — |
| DS1302 | CLK | GPIO **4** | — |
| DS1302 | DAT | GPIO **3** | — |
| DS1302 | RST (CE) | GPIO **2** | — |
| Buzzer (+) | — | GPIO **7** | buzzer ativo; (−) no GND |
| LED (ânodo) | — | GPIO **45** | em série com resistor de 220 Ω ao GND |

> **Ajuste de sensibilidade:** o módulo TCRT5000 tem um potenciômetro que regula a
> distância de detecção. Ajuste-o com o remédio na posição para que o LED indicador do
> módulo acenda com o remédio presente e apague quando ele for retirado.
>
> **Polaridade:** se a detecção ficar invertida (acusar retirada quando o remédio está
> presente), troque `#define SENSOR_ATIVO_EM_LOW true` para `false` no `dispenser.ino`.

### 2.2. Diagrama geral de ligação (dispenser)

```
            Heltec V3 (dispenser)
          +-----------------------+
  3V3 ----| 3V3           GPIO 6  |<--- D0  (TCRT5000)
  GND ----| GND           GPIO 4  |---- CLK (DS1302)
          |               GPIO 3  |<--> DAT (DS1302)
          |               GPIO 2  |---- RST (DS1302)
          |               GPIO 7  |---- Buzzer (+)
          |               GPIO 45 |---- LED (+ resistor 220R)
          |  [SX1262 e OLED internos — sem fios]  |
          +-----------------------+
```

> **Antena:** antes de ligar qualquer placa Heltec V3, **conecte a antena LoRa**.
> Transmitir sem antena pode danificar o rádio.

---

## 3. Antes de gravar

- **Gateway:** copie `gateway/config.exemplo.h` para `gateway/config.h` e preencha
  Wi-Fi e broker MQTT. (O `config.h` não vai para o Git — tem senha.)
  - Para teste rápido, pode usar o broker público `broker.hivemq.com`, porta `1883`.
- **Dispenser:** confira no topo do `dispenser.ino` o `DISPOSITIVO_ID`, o horário da
  dose (`DOSE_HORA`/`DOSE_MINUTO`) e a polaridade do sensor (`SENSOR_ATIVO_EM_LOW`).
- **Os dois lados devem ter os MESMOS parâmetros LoRa** (frequência, SF, BW, CR,
  sync word). Já estão iguais nos dois sketches — não altere um sem o outro.

---

## 4. Passo a passo de teste no laboratório (incremental)

Teste **uma coisa de cada vez**. Não tente montar tudo de uma vez — é muito mais
difícil achar o erro depois.

### Etapa A — Só o rádio LoRa (duas placas, sem nada ligado)

Objetivo: confirmar que as duas Heltec V3 conversam por LoRa.

1. Grave `gateway.ino` na **placa A** (com `config.h` preenchido) e abra o Monitor
   Serial (115200). Confira: "LoRa OK", "Wi-Fi OK", "MQTT OK".
2. Grave `dispenser.ino` na **placa B**. No boot ele envia um `heartbeat`.
3. **No Monitor Serial do gateway** deve aparecer `LoRa recebido: {"id":"disp-01",...}`
   com o RSSI/SNR.
4. ✓ Se o gateway mostrar o JSON recebido, o LoRa está funcionando.

> Dica: para forçar envios, você pode reiniciar o dispenser (botão RST) — a cada boot
> ele manda um heartbeat.

### Etapa B — Sensor infravermelho (no dispenser)

Objetivo: confirmar a detecção de presença/ausência do remédio.

1. Monte o TCRT5000 conforme a seção 2 (VCC em 3,3V, D0 no GPIO 6).
2. Com o remédio na posição, ajuste o **potenciômetro** do módulo até o LED indicador
   do próprio módulo acender (objeto detectado).
3. Grave o `dispenser.ino` e abra o Monitor Serial.
4. **Remova** o remédio: deve aparecer `Retirada detectada!` e o evento ser enviado.
5. ✓ Se a detecção estiver invertida, troque `SENSOR_ATIVO_EM_LOW` para `false`.

### Etapa C — RTC DS1302 (no dispenser)

1. Monte o DS1302 conforme a seção 2 (lembre da bateria do RTC).
2. Na **primeira gravação**, o firmware ajusta a hora pela hora de compilação.
3. Confira no OLED/Serial se o timestamp dos eventos está correto.
4. ✓ Desligue e religue a placa: a hora deve se manter (prova de que o RTC guarda).

### Etapa D — Alerta e lógica de dose

1. Ajuste `DOSE_HORA`/`DOSE_MINUTO` para 1–2 minutos à frente da hora atual.
2. Aguarde o horário: o buzzer + LED devem acionar e um evento `alerta_disparado`
   deve ser enviado.
3. **Não** retire o medicamento e aguarde a janela de `TOLERANCIA_MIN`: deve surgir
   um evento `atraso`. (Para testar rápido, baixe a tolerância para 1 minuto.)
4. ✓ Reponha os valores reais depois do teste.

### Etapa E — Ponta a ponta até o MQTT

1. Com gateway + dispenser funcionando, assine o broker em outro computador:
   ```bash
   mosquitto_sub -h broker.hivemq.com -t 'dispenser/+/evento' -v
   ```
2. Gere eventos no dispenser (retirada, alerta).
3. ✓ As mensagens JSON devem aparecer no `mosquitto_sub`. A partir daqui, a frente
   de **nuvem (Node-RED)** já consegue consumir os dados.

---

## 5. Problemas comuns

| Sintoma | Causa provável | Solução |
|---|---|---|
| Monitor Serial vazio | USB CDC desabilitado | Ligue **USB CDC On Boot: Enabled** e regrave |
| "ERRO LoRa" no OLED | parâmetros ou SPI | confira que os dois lados têm os mesmos parâmetros |
| Gateway não recebe nada | sync word / antena | confirme antena conectada e `LORA_SYNC_WORD` igual |
| Detecção invertida (acusa retirada com remédio presente) | polaridade do módulo | troque `SENSOR_ATIVO_EM_LOW` para `false` |
| Sensor nunca detecta | sensibilidade / distância | ajuste o potenciômetro do módulo; aproxime o remédio |
| Placa reinicia ao transmitir | antena ausente / fonte fraca | conecte a antena; use fonte USB boa |
| Hora do RTC errada/zerada | bateria do DS1302 | troque a bateria; regrave para reajustar |

---

## 6. Relação com a pontuação

- **Comunicação LoRa (obrigatório):** Etapas A e E.
- **Validação de circuito (1 pt):** Etapas B, C, D (sensor, RTC, alerta).
- **Etapa de nuvem com MQTT (2 pts):** Etapa E alimenta o Node-RED.
- **Extra — OTA:** a ser adicionado numa fase posterior.