#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- CONFIGURACIÓN DE RED ---
const char* ssid = "TU_RED_HOTSPOT";
const char* password = "TU_PASSWORD";
const char* mqtt_server = "IP_DE_TU_ARDUINO_UNO_Q"; // Ejemplo: 10.198.118.x

// --- PARÁMETROS DE PRECISIÓN ---
const int MPU_ADDR = 0x68;
const float UMBRAL_FUERZA = 1.8;  
const int CRUCES_MINIMOS = 8;     
const float UMBRAL_RUIDO = 0.25;  

WiFiClient espClient;
PubSubClient client(espClient);
QueueHandle_t colaDato;

float x_filtrada = 0, x_anterior = 0;
int contadorCruces = 0;
unsigned long ventanaTiempo = 0;
bool signoAnterior = false;

void despertarMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);
  Wire.endTransmission(true);
}

void leerSensor(float &accX) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2, true); 
  int16_t rawX = Wire.read() << 8 | Wire.read();
  accX = (rawX / 16384.0) * 9.81;
}

void TareaLeerSensor(void *pvParameters) {
  float ax;
  for (;;) {
    leerSensor(ax);

    x_filtrada = 0.94 * (x_filtrada + ax - x_anterior);
    x_anterior = ax;

    float dato_grafica = x_filtrada;
    if (abs(dato_grafica) < UMBRAL_RUIDO) dato_grafica = 0;

    bool signoActual = (dato_grafica > 0);
    if (abs(dato_grafica) > UMBRAL_FUERZA) { 
      if (signoActual != signoAnterior) {
        contadorCruces++;
        signoAnterior = signoActual;
      }
    }

    if (millis() - ventanaTiempo > 1000) {
      if (contadorCruces >= CRUCES_MINIMOS) {
        Serial.println("¡SISMO DETECTADO POR EL SENSOR!");
        if (client.connected()) {
          client.publish("sismo/alerta", "PELIGRO");
        }
      }
      contadorCruces = 0;
      ventanaTiempo = millis();
    }

    xQueueSend(colaDato, &dato_grafica, portMAX_DELAY);
    vTaskDelay(20 / portTICK_PERIOD_MS); 
  }
}

void TareaComunicacion(void *pvParameters) {
  float valor;
  unsigned long ultimoEnvio = 0;
  unsigned long ultimoIntento = 0;

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      if (millis() - ultimoIntento > 3000) {
        Serial.println("Buscando red WiFi...");
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        ultimoIntento = millis();
      }
    } 
    else if (!client.connected()) {
      if (millis() - ultimoIntento > 3000) {
        Serial.print("WiFi conectado. Buscando Broker en: ");
        Serial.println(mqtt_server);
        if (client.connect("PublisherESP32")) {
          Serial.println("¡Conectado al servidor MQTT!");
        } else {
          Serial.print("Error MQTT. Código: ");
          Serial.println(client.state());
        }
        ultimoIntento = millis();
      }
    } 
    else {
      client.loop();
      if (xQueueReceive(colaDato, &valor, 0) == pdPASS) {
        if (millis() - ultimoEnvio > 50) { 
          client.publish("sismo/grafica", String(valor).c_str());
          ultimoEnvio = millis();
        }
      }
    }

    if (WiFi.status() != WL_CONNECTED || !client.connected()) {
      xQueueReceive(colaDato, &valor, 0); 
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- Iniciando Nodo Sismológico ESP32 ---");
  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);
  Wire.begin(21, 22); 
  despertarMPU();
  colaDato = xQueueCreate(10, sizeof(float));
  
  xTaskCreatePinnedToCore(TareaLeerSensor, "Sensor", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TareaComunicacion, "Red", 4096, NULL, 1, NULL, 0);
}

void loop() {}