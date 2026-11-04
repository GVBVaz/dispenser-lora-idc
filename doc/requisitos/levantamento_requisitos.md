# Levantamento de Requisitos — Dispenser de Medicamentos LoRa

Documento de levantamento formal de requisitos do projeto final da disciplina de
**Internet das Coisas** (ICT/UNIFESP). Este é um dos itens **obrigatórios** da entrega.

> A versão em planilha (para o template do professor) está em
> [`levantamento_requisitos.csv`](./levantamento_requisitos.csv) — abra no Excel ou
> Google Sheets. Esta página Markdown é a versão legível para consulta no GitHub.

## 1. Visão geral do sistema

O sistema é um **dispenser de medicamentos monitorado** que detecta a retirada do
remédio, alerta o paciente nos horários das doses e envia a telemetria por rádio
**LoRa** até um **gateway**, que publica os eventos em um **broker MQTT**. Na nuvem, um
fluxo **Node-RED** consome os tópicos, exibe o status em um **dashboard** para cuidadores
e persiste o histórico em um banco **PostgreSQL**.

```
Nó Dispenser (ESP32 + SX1262)  --LoRa 915 MHz-->  Gateway (ESP32 + Wi-Fi)
                                                        |
                                                        v  MQTT
                                                  Broker MQTT
                                                        |
                                                        v
                                             Node-RED (dashboard + banco)
                                                        |
                                                        v
                                                   PostgreSQL
```

## 2. Decisões de projeto adotadas

| Aspecto | Decisão |
|---|---|
| Rádio LoRa | **SX1262** em **915 MHz** (banda ISM livre no Brasil) |
| Sensor de retirada | **Ultrassônico HC-SR04** (disponível no laboratório) |
| Compartimentos | **1** no protótipo (arquitetura escalável para N) |
| Alerta de dose | **Buzzer + LED** |
| Relógio de tempo real | **DS3231** (I2C) para timestamp confiável |
| Alimentação (dispenser) | **Li-ion 18650 + TP4056 + regulador 3,3 V** |
| Gateway | ESP32 + SX1262 + Wi-Fi, alimentado por USB |
| Configuração local | **BLE** apenas na configuração inicial |

## 3. Legenda das categorias

- **Funcional** — comportamento que o sistema deve apresentar.
- **Comunicação** — relativo à troca de dados (LoRa, MQTT).
- **Dimensional** — relativo a tamanho/encaixe físico (PCI, encapsulamento).
- **Segurança** — proteção elétrica e integridade do circuito.
- **Energia / Não Funcional** — consumo e autonomia.
- **Processo** — relativo ao desenvolvimento (versionamento).

## 4. Requisitos

Legenda de IDs: **RF** = Requisito Funcional · **RNF** = Requisito Não Funcional.

| ID | Requisito | Categoria | Autor(a) |
|---|---|---|---|
| RF-01 | Detectar a retirada do medicamento do compartimento | Funcional | Gabriel |
| RF-02 | Emitir alerta sonoro e visual no horário da dose | Funcional | Luiza |
| RF-03 | Registrar data e horário de cada retirada | Funcional | Robert |
| RF-04 | Classificar dose como "em atraso" fora da janela | Funcional | Robert |
| RF-05 | Transmitir eventos por LoRa até o gateway | Comunicação | Gabriel |
| RF-06 | Publicar eventos recebidos em broker MQTT | Comunicação | Luiza |
| RF-07 | Exibir status diário em dashboard | Funcional | Luiza |
| RF-08 | Permitir configuração inicial via BLE | Funcional | Gabriel |
| RF-09 | Armazenar histórico em banco relacional | Funcional | Robert |
| RNF-01 | PCI deve caber no encapsulamento | Dimensional | Gabriel |
| RNF-02 | Operar por bateria recarregável | Energia | Robert |
| RNF-03 | Proteção contra inversão de polaridade/sobrecorrente | Segurança | Gabriel |
| RNF-04 | Versionamento do código em Git | Processo | Robert |
| RNF-05 | Atualização remota de firmware (OTA) | Não Funcional | Gabriel |

A tabela completa — com **Razão**, **Especificação Técnica**, **Requisito de
Desempenho**, **Rotina de Teste** e **Critério de Aceitação** de cada requisito — está
na planilha [`levantamento_requisitos.csv`](./levantamento_requisitos.csv).

## 5. Rastreabilidade com a pontuação da disciplina

| Item da entrega | Requisitos relacionados |
|---|---|
| Validação de circuito (1 pt) | RF-01, RF-02, RNF-03 |
| Etapa de nuvem com MQTT (2 pts) | RF-06, RF-07 |
| Projeto e confecção da PCI (3 pts) | RNF-01, RNF-03 |
| Comunicação LoRa (obrigatório) | RF-05, RF-06 |
| Documentação (1 pt) | este documento, SysML |
| **Extra** — banco de dados | RF-09 |
| **Extra** — OTA | RNF-05 |
| **Extra** — encapsulamento 3D | RNF-01 |

## 6. Próximos passos de documentação

- [ ] Diagramas **SysML** (requisitos, blocos e caso de uso) citados pelo professor.
- [ ] Especificação do **protocolo de mensagens** LoRa + árvore de tópicos MQTT.
- [ ] Esquema do **banco de dados** (tabelas e relacionamentos).

> **Nota:** os valores de desempenho (ex.: autonomia de 24 h, alcance de 1 km) são metas
> de projeto. Os números finais devem ser ajustados conforme os resultados dos testes
> reais em laboratório antes da apresentação.