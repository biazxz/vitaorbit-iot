// =============================================
// VitaOrbit
// Monitor de Sinais Vitais de Astronautas
// =============================================
 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
 
// --- LCD ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
 
// --- Wi-Fi ---
const char* ssid     = "Wokwi-GUEST";
const char* password = "";
 
// --- MQTT ---
const char* mqttServer = "broker.hivemq.com";
const int   mqttPort   = 1883;
const char* mqttClient = "OrbitalHealthESP32";
 
WiFiClient   espClient;
PubSubClient mqtt(espClient);
 

#define PIN_POTENCIOMETRO 34
#define PIN_NTC           35
#define PIN_LED_VERDE     26
#define PIN_LED_AMARELO   25
#define PIN_LED_VERMELHO  27
 

int   batimento   = 0;
float temperatura = 0.0;
long  uptime      = 0;
 
// =============================================
// LEITURA DO POTENCIÔMETRO → BATIMENTO (BPM)
// =============================================
int lerBatimento() {
    int leitura = analogRead(PIN_POTENCIOMETRO);
    int bpm = map(leitura, 0, 4095, 40, 200);
    return bpm;
}
 
// =============================================
// LEITURA DO NTC → TEMPERATURA (°C)
// =============================================

float lerTemperatura() {

    const float BETA = 3950.0;     
    const float SERIES_RESISTOR = 10000.0;

    int leitura = analogRead(PIN_NTC);

    Serial.print("ADC NTC: ");
    Serial.println(leitura);

    if (leitura <= 0 || leitura >= 4095) {
        return 25.0;
    }

    float resistencia = SERIES_RESISTOR /
                        ((4095.0 / leitura) - 1.0);

    float temperaturaK =
        1.0 / (log(resistencia / 10000.0) / BETA +
               1.0 / 298.15);

    float temperaturaC = temperaturaK - 273.15;

    Serial.print("Temperatura calculada: ");
    Serial.println(temperaturaC);

    return temperaturaC;
}
 
// =============================================
// VERIFICA STATUS DA PESSOA MONITORADA
// =============================================
String verificarStatus(int bpm, float temp) {
    if ((bpm > 120 || bpm < 40) && temp > 38.5) {
        return "CRITICO";
    }
    if (bpm > 120 || bpm < 40) {
        return "ATENCAO_BPM";
    }
    if (temp > 38.5 || temp < 36.6) {
        return "ATENCAO_TEMP";
    }
    return "NORMAL";
}
 
// =============================================
// ATUALIZA O LCD
// =============================================
void atualizarLCD(int bpm, float temp, String status) {
    lcd.clear();
 
    lcd.setCursor(0, 0);
    lcd.print("BPM:");
    lcd.print(bpm);
    lcd.print(" T:");
    lcd.print(temp, 1);
    lcd.print("C");
 
    lcd.setCursor(0, 1);
    if (status == "CRITICO") {
        lcd.print("!! CRITICO !!");
    } else if (status == "ATENCAO_BPM") {
        lcd.print("ATENCAO: BPM!");
    } else if (status == "ATENCAO_TEMP") {
        lcd.print("ATENCAO: TEMP!");
    } else {
        lcd.print("Status: NORMAL");
    }
}
 
// =============================================
// CONTROLA OS LEDs
// =============================================
void controlarLEDs(String status) {
    // Apaga todos primeiro
    digitalWrite(PIN_LED_VERDE,    LOW);
    digitalWrite(PIN_LED_AMARELO,  LOW);
    digitalWrite(PIN_LED_VERMELHO, LOW);
 
    if (status == "CRITICO") {
        digitalWrite(PIN_LED_VERMELHO, HIGH);
    } else if (status == "ATENCAO_BPM" || status == "ATENCAO_TEMP") {
        digitalWrite(PIN_LED_AMARELO,  HIGH);
    } else {
        digitalWrite(PIN_LED_VERDE,    HIGH);
    }
}
 
// =============================================
// CONECTA WI-FI
// =============================================
void conectarWiFi() {
    WiFi.begin(ssid, password);
 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conectando WiFi");
 
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }
 
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi conectado!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi conectado!");
        lcd.setCursor(0, 1);
        lcd.print(WiFi.localIP());
        delay(2000);
    } else {
        Serial.println("\nFalha no Wi-Fi!");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Falha no WiFi!");
        delay(2000);
    }
}
 
// =============================================
// CONECTA MQTT
// =============================================
void conectarMQTT() {
    mqtt.setServer(mqttServer, mqttPort);
 
    while (!mqtt.connected()) {
        Serial.print("Conectando MQTT...");
 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Conectando MQTT");
 
        if (mqtt.connect(mqttClient)) {
            Serial.println("MQTT conectado!");
 
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("MQTT conectado!");
            delay(2000);
        } else {
            Serial.print("Falha MQTT: ");
            Serial.println(mqtt.state());
            delay(3000);
        }
    }
}
 
// =============================================
// PUBLICA NOS TÓPICOS MQTT
// =============================================
void publicarMQTT(int bpm, float temp, String status) {
    if (!mqtt.connected()) {
        conectarMQTT();
    }
 
    char payload[200];
 
    snprintf(payload, sizeof(payload),
        "{\"batimento\":%d,\"temperatura\":%.1f,\"status\":\"%s\",\"astronauta\":\"VitaOrbit\"}",
        bpm, temp, status.c_str()
    );
    mqtt.publish("orbital/sinais", payload);
    Serial.print("orbital/sinais → ");
    Serial.println(payload);
 
    if (status == "CRITICO" || status == "ATENCAO_BPM" || status == "ATENCAO_TEMP") {
        if (bpm > 120) {
            snprintf(payload, sizeof(payload),
                "{\"tipo\":\"%s\",\"mensagem\":\"Batimento alto: %dbpm\",\"temperatura\":%.1f}",
                status.c_str(), bpm, temp
            );
        } else if (bpm < 40) {
            snprintf(payload, sizeof(payload),
                "{\"tipo\":\"%s\",\"mensagem\":\"Batimento baixo: %dbpm\",\"temperatura\":%.1f}",
                status.c_str(), bpm, temp
            );
        } else {
            snprintf(payload, sizeof(payload),
                "{\"tipo\":\"%s\",\"mensagem\":\"Febre detectada: %.1fC\",\"temperatura\":%.1f}",
                status.c_str(), temp, temp
            );
        }
        mqtt.publish("orbital/alertas", payload);
        Serial.print("orbital/alertas → ");
        Serial.println(payload);
    }
 
    uptime = millis() / 1000;
    snprintf(payload, sizeof(payload),
        "{\"dispositivo\":\"ESP32\",\"missao\":\"Artemis VII\",\"uptime_seg\":%ld}",
        uptime
    );
    mqtt.publish("orbital/status", payload);
    Serial.print("orbital/status → ");
    Serial.println(payload);
}
 
// =============================================
// SETUP
// =============================================
void setup() {
    Serial.begin(115200);
 
    pinMode(PIN_LED_VERDE,    OUTPUT);
    pinMode(PIN_LED_AMARELO,  OUTPUT);
    pinMode(PIN_LED_VERMELHO, OUTPUT);
 
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Orbital Health");
    lcd.setCursor(0, 1);
    lcd.print("Monitor v1.0");
    delay(2000);
 
    conectarWiFi();
    conectarMQTT();
 
    Serial.println("Sistema iniciado!");
}
 
// =============================================
// LOOP
// =============================================
void loop() {
    mqtt.loop();
 
    batimento   = lerBatimento();
    temperatura = lerTemperatura();
 
    String status = verificarStatus(batimento, temperatura);
 
    atualizarLCD(batimento, temperatura, status);
    controlarLEDs(status);
    publicarMQTT(batimento, temperatura, status);
 
    Serial.println("================================");
    Serial.print("BPM: ");
    Serial.println(batimento);
    Serial.print("Temperatura: ");
    Serial.print(temperatura, 1);
    Serial.println("°C");
    Serial.print("Status: ");
    Serial.println(status);
    Serial.println("================================");
 
    delay(3000);
}
