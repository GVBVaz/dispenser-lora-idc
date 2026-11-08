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

### 1.3. Configuração ao gravar (Ferramentas)

- **Placa:** Heltec WiFi LoRa 32(V3)
- **Upload Speed:** 921600 (se der erro de upload, baixe para 115200)
- **USB CDC On Boot:** Enabled *(importante para ver o Monitor Serial)*
- **Porta:** a COM/tty que aparecer ao conectar a placa via USB

---

## 2. Ligações de hardware (apenas no nó DISPENSER)

O **gateway não precisa de ligação nenhuma** — ele só usa o rádio e o OLED, que já
são internos da Heltec V3. Basta alimentá-lo por USB.

No **dispenser**, ligamos o sensor ultrassônico, o RTC e o alerta. O rádio e o OLED
já são internos (não precisam de fios).

### 2.1. Tabela de ligações

| Componente | Pino do componente | GPIO na Heltec V3 | Observação |
|---|---|---|---|
| HC-SR04 | VCC | **5V** (pino `5V`/`VBUS`) | o sensor precisa de 5V |
| HC-SR04 | GND | GND | — |
| HC-SR04 | TRIG | GPIO **5** | ligação direta |
| HC-SR04 | ECHO | GPIO **6** | **⚠ via divisor de tensão** (ver 2.2) |
| DS1302 | VCC | 3V3 | — |
| DS1302 | GND | GND | — |
| DS1302 | CLK | GPIO **4** | — |
| DS1302 | DAT | GPIO **3** | — |
| DS1302 | RST (CE) | GPIO **2** | — |
| Buzzer (+) | — | GPIO **7** | buzzer ativo; (−) no GND |
| LED (ânodo) | — | GPIO **45** | em série com resistor de 220 Ω ao GND |

### 2.2. ⚠ Divisor de tensão no ECHO (NÃO PULE ISTO)

O pino **ECHO do HC-SR04 devolve 5V**, mas os GPIOs do ESP32-S3 **só toleram 3,3V**.
Ligar o ECHO direto pode **danificar a placa**. Use dois resistores para dividir a
tensão (ex.: R1 = 1 kΩ e R2 = 2 kΩ):

```
   ECHO (5V) ----[ R1 = 1k ]----+----[ R2 = 2k ]---- GND
                                |
                                +-----> GPIO 6 (chega ~3,3V)
```

> Regra: a tensão no GPIO = 5V × R2/(R1+R2) = 5 × 2/3 ≈ 3,3V. ✔

### 2.3. Diagrama geral de ligação (dispenser)

```
            Heltec V3 (dispenser)
          +-----------------------+
   5V ----| 5V            GPIO 5  |---- TRIG (HC-SR04)
  GND ----| GND           GPIO 6  |<--- ECHO (via divisor 1k/2k)
  3V3 ----| 3V3           GPIO 4  |---- CLK  (DS1302)
          |               GPIO 3  |<--> DAT  (DS1302)
          |               GPIO 2  |---- RST  (DS1302)
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
  dose (`DOSE_HORA`/`DOSE_MINUTO`) e o `LIMIAR_PRESENCA_CM`.
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
4. ✔ Se o gateway mostrar o JSON recebido, o LoRa está funcionando.

> Dica: para forçar envios, você pode reiniciar o dispenser (botão RST) — a cada boot
> ele manda um heartbeat.

### Etapa B — Sensor ultrassônico (no dispenser)

1. Monte o HC-SR04 conforme a seção 2 (**com o divisor de tensão no ECHO**).
2. Grave o `dispenser.ino` e abra o Monitor Serial.
3. Aproxime e afaste um objeto do sensor. Ao **remover** o objeto (distância passa de
   ≤10 cm para >10 cm), deve aparecer `Retirada detectada!` e o evento ser enviado.
4. ✔ Ajuste `LIMIAR_PRESENCA_CM` se necessário para a sua caixa.

### Etapa C — RTC DS1302 (no dispenser)

1. Monte o DS1302 conforme a seção 2 (lembre da bateria do RTC).
2. Na **primeira gravação**, o firmware ajusta a hora pela hora de compilação.
3. Confira no OLED/Serial se o timestamp dos eventos está correto.
4. ✔ Desligue e religue a placa: a hora deve se manter (prova de que o RTC guarda).

### Etapa D — Alerta e lógica de dose

1. Ajuste `DOSE_HORA`/`DOSE_MINUTO` para 1–2 minutos à frente da hora atual.
2. Aguarde o horário: o buzzer + LED devem acionar e um evento `alerta_disparado`
   deve ser enviado.
3. **Não** retire o medicamento e aguarde a janela de `TOLERANCIA_MIN`: deve surgir
   um evento `atraso`. (Para testar rápido, baixe a tolerância para 1 minuto.)
4. ✔ Reponha os valores reais depois do teste.

### Etapa E — Ponta a ponta até o MQTT

1. Com gateway + dispenser funcionando, assine o broker em outro computador:
   ```bash
   mosquitto_sub -h broker.hivemq.com -t 'dispenser/+/evento' -v
   ```
2. Gere eventos no dispenser (retirada, alerta).
3. ✔ As mensagens JSON devem aparecer no `mosquitto_sub`. A partir daqui, a frente
   de **nuvem (Node-RED)** já consegue consumir os dados.

---

## 5. Problemas comuns

| Sintoma | Causa provável | Solução |
|---|---|---|
| Monitor Serial vazio | USB CDC desabilitado | Ligue **USB CDC On Boot: Enabled** e regrave |
| "ERRO LoRa" no OLED | parâmetros ou SPI | confira que os dois lados têm os mesmos parâmetros |
| Gateway não recebe nada | sync word / antena | confirme antena conectada e `LORA_SYNC_WORD` igual |
| Distância sempre -1 | ligação do HC-SR04 | revise TRIG/ECHO e o divisor; confira VCC em 5V |
| Placa reinicia ao transmitir | antena ausente / fonte fraca | conecte a antena; use fonte USB boa |
| Hora do RTC errada/zerada | bateria do DS1302 | troque a bateria; regrave para reajustar |

---

## 6. Relação com a pontuação

- **Comunicação LoRa (obrigatório):** Etapas A e E.
- **Validação de circuito (1 pt):** Etapas B, C, D (sensor, RTC, alerta).
- **Etapa de nuvem com MQTT (2 pts):** Etapa E alimenta o Node-RED.
- **Extra — OTA:** a ser adicionado numa fase posterior.