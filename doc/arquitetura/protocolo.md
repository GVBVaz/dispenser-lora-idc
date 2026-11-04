# Contrato de Dados — Protocolo LoRa e Tópicos MQTT

Este documento define o **contrato de dados** do projeto: o formato das mensagens
trafegadas por LoRa, a árvore de tópicos MQTT e os campos persistidos no banco. É a
referência única para as três frentes do projeto (firmware, nuvem e banco) — todas
devem seguir exatamente o que está aqui descrito para que a integração funcione.

> **Regra de ouro:** se o formato precisar mudar, muda-se **este documento primeiro** e
> as três frentes se ajustam a ele. Ninguém inventa campos por conta própria.

## 1. Fluxo de ponta a ponta

```
Nó Dispenser                Gateway                 Broker MQTT        Node-RED
(ESP32 + SX1262)            (ESP32 + Wi-Fi)                            (nuvem)
      |                          |                        |                |
      |  pacote LoRa (JSON)      |                        |                |
      |------------------------->|                        |                |
      |                          |  publish MQTT          |                |
      |                          |----------------------->|   subscribe    |
      |                          |                        |--------------->|
      |                          |                        |         grava no banco
      |                          |                        |         atualiza dashboard
```

## 2. Payload LoRa (dispenser → gateway)

O nó dispenser transmite cada evento como um **JSON compacto** (chaves curtas para
economizar bytes no ar, já que o LoRa tem payload limitado). Exemplo:

```json
{"id":"disp-01","evt":"retirada","ts":"2026-11-02T08:15:00","comp":1,"fw":"1.0.0"}
```

### Campos

| Campo | Tipo | Obrigatório | Descrição |
|---|---|---|---|
| `id` | string | sim | Identificador único do dispenser (ex.: `disp-01`). |
| `evt` | string | sim | Tipo do evento (ver tabela abaixo). |
| `ts` | string | sim | Timestamp ISO-8601 local lido do RTC (`YYYY-MM-DDTHH:MM:SS`). |
| `comp` | inteiro | sim | Número do compartimento (1 no protótipo; preparado para N). |
| `fw` | string | não | Versão do firmware (útil para o desafio de OTA). |

### Tipos de evento (`evt`)

| Valor | Quando é enviado |
|---|---|
| `retirada` | O medicamento foi retirado do compartimento (sensor detectou ausência). |
| `alerta_disparado` | O alerta de dose (buzzer + LED) foi acionado no horário programado. |
| `atraso` | A dose não foi retirada dentro da janela de tolerância. |
| `heartbeat` | Sinal de vida periódico (confirma que o dispositivo está operante). |

> **Nota de implementação (firmware):** manter as chaves curtas (`id`, `evt`, `ts`,
> `comp`) e evitar espaços no JSON reduz o tamanho do pacote LoRa. O payload de exemplo
> acima tem ~70 bytes, bem dentro do limite típico do SX1262.

## 3. Árvore de tópicos MQTT (gateway → broker)

O gateway publica cada evento recebido reencaminhando o mesmo JSON do payload LoRa.

| Tópico | Retido? | Payload | Descrição |
|---|---|---|---|
| `dispenser/<id>/evento` | não | JSON do evento (seção 2) | Um por evento recebido via LoRa. |
| `dispenser/<id>/status` | **sim** | `{"online":true,"ts":"..."}` | Presença do dispositivo (online/offline). |

- `<id>` é substituído pelo `id` do dispenser (ex.: `dispenser/disp-01/evento`).
- O tópico `status` usa a flag **retida** para que novos assinantes recebam o último
  estado conhecido ao se conectarem.
- **QoS sugerido:** 1 (ao menos uma entrega) para `evento`; garante que a nuvem não
  perca retiradas e atrasos.

### Exemplo de assinatura (teste rápido)

```bash
mosquitto_sub -h <broker> -t 'dispenser/+/evento' -v
```

## 4. Mapeamento evento → banco de dados

Cada mensagem publicada em `dispenser/<id>/evento` vira uma linha na tabela `evento`.
O esquema completo está em [`../banco/esquema_banco.sql`](../banco/esquema_banco.sql).

| Campo do JSON | Coluna na tabela `evento` |
|---|---|
| `id` | `dispositivo_id` (FK para `dispositivo`) |
| `evt` | `tipo` |
| `ts` | `ocorrido_em` |
| `comp` | `compartimento` |
| `fw` | `firmware_versao` |
| (hora de recepção na nuvem) | `recebido_em` |

## 5. Versionamento do contrato

| Versão | Data | Mudança |
|---|---|---|
| 1.0 | 2026-11 | Versão inicial: payload JSON, tópicos MQTT, mapeamento para o banco. |

> Qualquer alteração de campo ou tópico incrementa a versão e é comunicada às três
> frentes.