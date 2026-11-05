# Plano de Projeto — Dispenser de Medicamentos LoRa

Documento de planejamento e divisão de tarefas do projeto final de Internet das Coisas
(ICT/UNIFESP). Serve como guia para a equipe e como registro das decisões.

## 1. Integrantes e frentes de trabalho

| Integrante | Frente principal | Responsabilidades |
|---|---|---|
| **Gabriel Vaz Baruel** | Firmware + PCI | Firmware do dispenser e do gateway, projeto e confecção da PCI, OTA. |
| **Luiza Dugois Silva Alves** | Nuvem | Broker MQTT, fluxo Node-RED, dashboard de monitoramento. |
| **Robert Angelo de Souza Santos** | Banco + Documentação | Esquema PostgreSQL, integração Node-RED↔banco e documentação. |

> As frentes são o foco principal de cada um, mas o trabalho é colaborativo. O importante
> é que **cada pessoa tenha commits próprios** para comprovar a contribuição individual
> (a apresentação é individual e exige descrever o que cada um fez).

## 2. Checklist de itens OBRIGATÓRIOS

> Se faltar qualquer um destes, o projeto é considerado **não entregue**.

- [x] Levantamento de requisitos — *planilha pronta*
- [x] Versionamento de código no Git — *repositório criado e em uso*
- [ ] Etapa de nuvem em Node-RED
- [ ] Uso de comunicação LoRa
- [ ] Documentação do projeto da PCI
- [ ] Apresentação com evidências de funcionamento

## 3. Pontuação-alvo

| Item | Pontos | Status |
|---|---|---|
| Validação de circuito | 1 | a fazer |
| Etapa de nuvem com MQTT | 2 | a fazer |
| Projeto e confecção da PCI | 3 | a fazer |
| Documentação | 1 | em andamento |
| Apresentação e arguição (individual) | 3 | a fazer |
| **Extra** — banco de dados para histórico | +1 | planejado |
| **Extra** — atualização remota de firmware (OTA) | +1 | planejado |
| **Extra** — encapsulamento impresso em 3D | +1 | planejado |

## 4. Fases do projeto

### Fase 0 — Fundação (em andamento)
- [x] Estrutura do repositório
- [x] Levantamento de requisitos
- [x] Contrato de dados (protocolo LoRa + tópicos MQTT + schema do banco)
- [ ] Diagramas SysML (requisitos, blocos, caso de uso)

### Fase 1 — Desenvolvimento em paralelo (desbloqueado pelo contrato de dados)
- [ ] **Firmware dispenser:** ler sensor ultrassônico, RTC, alerta, enviar LoRa
- [ ] **Firmware gateway:** receber LoRa, conectar Wi-Fi, publicar MQTT
- [ ] **Nuvem:** broker MQTT + fluxo Node-RED + dashboard
- [ ] **Banco:** aplicar schema PostgreSQL + gravação via Node-RED
- [ ] **PCI:** esquemático e layout no KiCad

### Fase 2 — Integração e extras
- [ ] Integração ponta a ponta (dispenser → gateway → nuvem → banco → dashboard)
- [ ] Confecção física da PCI
- [ ] Encapsulamento 3D
- [ ] OTA
- [ ] Testes conforme os critérios de aceitação da planilha de requisitos

### Fase 3 — Entrega
- [ ] Documentação final (incl. documentação da PCI)
- [ ] Coleta de evidências de funcionamento (fotos, vídeos, prints do dashboard)
- [ ] Preparação da apresentação individual

## 5. Convenções de trabalho

- **Branches por tarefa:** `feat/...`, `docs/...`, `fix/...`. Nunca commitar direto na `main`.
- **Pull Requests** revisados antes do merge na `main`.
- **Commits no padrão convencional:** `feat:`, `fix:`, `docs:`, `chore:`.
- **Contrato de dados é fonte única:** mudanças de formato alteram o
  [`doc/arquitetura/protocolo.md`](./arquitetura/protocolo.md) primeiro.

## 6. Artefatos de referência

| Documento | Caminho |
|---|---|
| Requisitos (planilha) | `doc/requisitos/IoT - Requisitos, Especificações Técnicas e Testes.xlsx` |
| Requisitos (legível) | `doc/requisitos/levantamento_requisitos.md` |
| Contrato de dados | `doc/arquitetura/protocolo.md` |
| Esquema do banco | `doc/banco/esquema_banco.sql` |