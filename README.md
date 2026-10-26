# Smart Medicine Dispenser - Internet das Coisas (UNIFESP)

Projeto final desenvolvido para a disciplina de Internet das Coisas do ICT/UNIFESP. O sistema consiste em um dispenser de medicamentos monitorado via rádio LoRa, integrado a um gateway MQTT, dashboard Node-RED e banco de dados para histórico de adesão ao tratamento.

## 👥 Integrantes do Grupo
* **Integrante 1** - Gabriel Vaz Baruel - 176.323
* **Integrante 2** - Luiza Dugois Silva Alves - 177.115
* **Integrante 2** - Robert Angelo de Souza Santos - 163.984

## 📁 Estrutura do Repositório
* `/firmware/dispenser`: Código C++ (ESP32 + LoRa SX1262) do nó sensor acoplado à gaveta.
* `/firmware/gateway`: Código C++ (ESP32 + LoRa SX1262 + Wi-Fi) do nó concentrador MQTT.
* `/hardware/pcb`: Esquemáticos e layouts de Placa de Circuito Impresso (Gerber, KiCad/EasyEDA).
* `/hardware/3d_model`: Modelos `.STEP` e `.STL` do gabinete para impressão 3D.
* `/cloud/nodered`: Fluxos `.json` do Node-RED e scripts SQL do Banco de Dados.
* `/doc`: Documentação do projeto e relatório acadêmico em LaTeX (`.tex`).

## 🚀 Como Executar
1. Clone este repositório: `git clone <URL_DO_REPOSITORIO>`
2. Abra os diretórios de firmware na sua IDE (VS Code/PlatformIO ou Arduino IDE).
3. Importe o arquivo `.json` em `/cloud/nodered` para a sua instância do Node-RED.