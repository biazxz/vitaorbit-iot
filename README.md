
# 🛰️ VitaOrbit — Protótipo IoT

> Monitoramento de sinais vitais de astronautas em tempo real

**Global Solution FIAP 2026 — 2TDSPI**

Integrantes:
- Beatriz de Sousa Franco — RM563686
- Giovana Souza Vieira — RM564430
- Maria Fernanda Mendes — RM565277
- Natan Freitas de Moraes — RM564992

---

## 📋 Sobre o projeto

O VitaOrbit é um sistema IoT que simula o monitoramento de sinais vitais
de astronautas em missões espaciais. O dispositivo coleta batimento cardíaco
e temperatura corporal, exibe os dados em um LCD local, acende LEDs de
alerta e transmite as informações via MQTT para um dashboard em tempo real e uso das bibliotecas LiquidCrystal I2C e PubSubClient.

---

## 🔧 Hardware utilizado

| Componente | Função |
|-----------|--------|
| ESP32 DevKit V1 | Microcontrolador principal |
| Potenciômetro | Simula batimento cardíaco (40–200 bpm) |
| NTC Thermistor | Simula temperatura corporal (35–42°C) |
| LED Verde | Status NORMAL |
| LED Amarelo | Status ATENÇÃO |
| LED Vermelho | Status CRÍTICO |
| LCD 16x2 I2C | Interface local de visualização |

---

## 📡 Tópicos MQTT

**Broker:** `broker.hivemq.com` | **Porta:** `1883`

### orbital/sinais
Publicado a cada 3 segundos com os dados dos sensores.
```json
{
  "batimento": 98,
  "temperatura": 36.5,
  "status": "NORMAL",
  "astronauta": "VitaOrbit"
}
```

### orbital/alertas
Publicado quando algum sinal está fora do padrão.
```json
{
  "tipo": "ATENCAO_BPM",
  "mensagem": "Batimento alto: 125bpm",
  "temperatura": 37.1
}
```

### orbital/status
Publicado a cada 3 segundos com dados do dispositivo.
```json
{
  "dispositivo": "ESP32",
  "missao": "Artemis VII",
  "uptime_seg": 120
}
```

---

## 🚦 Regras de alerta

| Condição | Status | LED |
|----------|--------|-----|
| BPM 40–120 e Temp 36.3–38.5°C | NORMAL | 🟢 Verde |
| Só batimento fora do padrão | ATENÇÃO | 🟡 Amarelo |
| Só temperatura fora do padrão | ATENÇÃO | 🟡 Amarelo |
| Ambos fora do padrão | CRÍTICO | 🔴 Vermelho |

---

## 🖥️ Simulação

Acesse a simulação no Wokwi:
🔗 [Link do Wokwi](https://wokwi.com/projects/305569599398609473)

---

## 📊 Dashboard

Visualize os dados em tempo real:
1. Acesse: https://www.hivemq.com/demos/websocket-client/
2. Clique em **Connect**
3. Subscreva no tópico: `orbital/#`

---

## 🎥 Vídeo

🔗 [Link do vídeo](https://youtu.be/CGsVQfU0WaU)

---

## 🗂️ Estrutura do repositório

```
vitaorbit-iot/
├── sketch.ino      ← Código do ESP32
├── diagram.json    ← Circuito Wokwi
└── README.md       ← Documentação
```
