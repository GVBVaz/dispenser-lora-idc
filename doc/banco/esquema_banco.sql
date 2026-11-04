-- ============================================================================
-- Esquema do Banco de Dados — Dispenser de Medicamentos LoRa
-- SGBD: PostgreSQL
--
-- Mantém o histórico de adesão ao tratamento (desafio de pontuação extra).
-- Os campos espelham o contrato de dados em doc/arquitetura/protocolo.md.
-- O Node-RED insere cada evento recebido via MQTT na tabela "evento".
-- ============================================================================

-- --------------------------------------------------------------------------
-- Tabela: dispositivo
-- Cadastro dos dispensers monitorados.
-- --------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS dispositivo (
    id              TEXT PRIMARY KEY,                 -- ex.: 'disp-01' (= campo "id" do JSON)
    nome            TEXT NOT NULL,                    -- nome amigável, ex.: 'Dispenser da sala'
    paciente        TEXT,                             -- nome/identificação do paciente
    criado_em       TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- --------------------------------------------------------------------------
-- Tabela: dose_programada
-- Horários em que cada compartimento deve ser utilizado.
-- --------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS dose_programada (
    id              SERIAL PRIMARY KEY,
    dispositivo_id  TEXT NOT NULL REFERENCES dispositivo(id),
    compartimento   INTEGER NOT NULL,                 -- 1 no protótipo
    medicamento     TEXT,                             -- nome do medicamento (opcional)
    horario         TIME NOT NULL,                    -- horário da dose, ex.: '08:00'
    tolerancia_min  INTEGER NOT NULL DEFAULT 30,      -- janela de tolerância (minutos)
    ativo           BOOLEAN NOT NULL DEFAULT TRUE
);

-- --------------------------------------------------------------------------
-- Tabela: evento
-- Histórico de todos os eventos recebidos dos dispensers.
-- --------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS evento (
    id               BIGSERIAL PRIMARY KEY,
    dispositivo_id   TEXT NOT NULL REFERENCES dispositivo(id),   -- campo "id" do JSON
    tipo             TEXT NOT NULL,                              -- campo "evt": retirada | alerta_disparado | atraso | heartbeat
    compartimento    INTEGER,                                   -- campo "comp"
    ocorrido_em      TIMESTAMPTZ NOT NULL,                       -- campo "ts" (horário do RTC no dispositivo)
    recebido_em      TIMESTAMPTZ NOT NULL DEFAULT now(),         -- quando a nuvem recebeu
    firmware_versao  TEXT,                                       -- campo "fw"
    CONSTRAINT tipo_valido CHECK (
        tipo IN ('retirada', 'alerta_disparado', 'atraso', 'heartbeat')
    )
);

-- Índices para consultas comuns do dashboard (histórico por dispositivo/data).
CREATE INDEX IF NOT EXISTS idx_evento_dispositivo ON evento (dispositivo_id);
CREATE INDEX IF NOT EXISTS idx_evento_ocorrido_em ON evento (ocorrido_em);
CREATE INDEX IF NOT EXISTS idx_evento_tipo        ON evento (tipo);

-- ============================================================================
-- Dados de exemplo (seed) — úteis para testar o dashboard sem hardware.
-- ============================================================================
INSERT INTO dispositivo (id, nome, paciente)
VALUES ('disp-01', 'Dispenser da sala', 'Paciente de teste')
ON CONFLICT (id) DO NOTHING;

INSERT INTO dose_programada (dispositivo_id, compartimento, medicamento, horario)
VALUES
    ('disp-01', 1, 'Medicamento A', '08:00'),
    ('disp-01', 1, 'Medicamento A', '20:00')
ON CONFLICT DO NOTHING;

-- ============================================================================
-- Consultas úteis (referência para o fluxo Node-RED)
-- ============================================================================

-- Eventos de hoje de um dispositivo:
-- SELECT tipo, compartimento, ocorrido_em
-- FROM evento
-- WHERE dispositivo_id = 'disp-01' AND ocorrido_em::date = CURRENT_DATE
-- ORDER BY ocorrido_em;

-- Total de retiradas por dia (adesão ao longo do tempo):
-- SELECT ocorrido_em::date AS dia, COUNT(*) AS retiradas
-- FROM evento
-- WHERE dispositivo_id = 'disp-01' AND tipo = 'retirada'
-- GROUP BY dia ORDER BY dia;