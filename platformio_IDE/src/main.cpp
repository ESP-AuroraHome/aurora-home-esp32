#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SensirionI2cScd30.h>
#include <BH1750.h>
#include <Adafruit_BME280.h>

// --- Configuration ---
const char *ssid = "ESP32-AP-Broker";  // Wi-Fi Name created by ESP32
const char *password = "password123";  // Wi-Fi Password
const int mqtt_port = 1883;
const char* mqtt_topic = "sensor/data";

// --- Global Objects ---
SensirionI2cScd30 scd30;
BH1750 bh1750(0x23);
Adafruit_BME280 bme280;
WiFiClient espClient;
PubSubClient client(espClient);

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

// ==========================================
// --- SENSOR FUNCTIONS ---
// ==========================================

bool scd30_init(SensirionI2cScd30 &scd30) {
    scd30.begin(Wire, SCD30_I2C_ADDR_61);
    scd30.stopPeriodicMeasurement();
    scd30.softReset();
    delay(2000);
    uint8_t major = 0, minor = 0;
    int16_t error = scd30.readFirmwareVersion(major, minor);
    if (error != NO_ERROR) return false;
    error = scd30.startPeriodicMeasurement(0); 
    if (error != NO_ERROR) return false;
    return true;
}

// Modified to return true ONLY if new data is actually ready
bool scd30_read(SensirionI2cScd30 &scd30, float &co2, float &temp, float &hum) {
  uint16_t dataReady = 0;
  scd30.getDataReady(dataReady);
  if (dataReady) {
    scd30.readMeasurementData(co2, temp, hum);
    return true;
  }
  return false;
}

bool bh1750_read(BH1750 &lightMeter, float &light) {
  if (lightMeter.measurementReady()) {
    light = lightMeter.readLightLevel();
    return true;
  }
  return false;
}

bool bme280_init(Adafruit_BME280 &bme) {
    if (!bme.begin(0x76)) { 
        if (!bme.begin(0x77)) return false; 
    }
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X2, 
                    Adafruit_BME280::SAMPLING_X16, 
                    Adafruit_BME280::SAMPLING_X1, 
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_500);
    return true;
}

bool bme280_read(Adafruit_BME280 &bme, float &temp, float &hum, float &pres) {
    temp = bme.readTemperature();
    hum = bme.readHumidity();
    pres = bme.readPressure() / 100.0F; 
    if (isnan(temp) || isnan(hum) || isnan(pres)) return false;
    return true;
}

// ==========================================
// --- MQTT & NETWORK LOGIC ---
// ==========================================

void reconnect() {
  int ipSuffixes[] = {2, 3, 4, 5}; 
  
  while (!client.connected()) {
    if (WiFi.softAPgetStationNum() == 0) {
        Serial.println("Waiting for Wi-Fi client (Orange Pi)...");
        delay(1000);
        return; 
    }

    Serial.println("--- Scanning for MQTT Broker (Orange Pi) ---");

    for (int i = 0; i < 4; i++) {
        IPAddress targetIP(192, 168, 4, ipSuffixes[i]);
        Serial.print("Trying IP: "); Serial.print(targetIP); Serial.print(" ... ");

        client.setServer(targetIP, mqtt_port);
        String clientId = "ESP32Client-" + String(random(0xffff), HEX);

        if (client.connect(clientId.c_str())) {
            Serial.println("✅ CONNECTED!");
            return; 
        } else {
            Serial.print("Failed (rc="); Serial.print(client.state()); Serial.println(")");
        }
    }
    Serial.println("Broker not found. Retrying in 3s...");
    delay(3000);
  }
}

void publish_data(float temp, float hum, float pres, float co2, float light) {
    char msg[512]; 
    
    snprintf(msg, sizeof(msg), 
             "{"
             "\"temperature\": \"%.2f °C\", "
             "\"humidity\": \"%.2f %%\", "
             "\"pressure\": \"%.2f hPa\", "
             "\"co2\": \"%.0f ppm\", "
             "\"light\": \"%.0f lx\""
             "}", 
             temp, hum, pres, co2, light);

    Serial.print("Publishing MQTT: ");
    Serial.println(msg);
    
    client.publish(mqtt_topic, msg);
}

// ==========================================
// --- SETUP ---
// ==========================================

void setup() {
  Serial.begin(115200);
  while (!Serial) {delay(100);}

  Wire.begin(21, 22); 

  Serial.println("\n--- 1. Sensor Initialization ---");
  if (bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) Serial.println(F("BH1750 OK"));
  else Serial.println(F("BH1750 ERROR"));

  if (scd30_init(scd30)) Serial.println("SCD30 OK");
  else { Serial.println("SCD30 ERROR"); while(1) delay(1000); }
  
  if (bme280_init(bme280)) Serial.println("BME280 OK");
  else { Serial.println("BME280 ERROR"); while(1) delay(1000); }

  Serial.println("\n--- 2. Starting Access Point ---");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP Created. SSID: "); Serial.println(ssid);
  Serial.print("ESP32 IP: "); Serial.println(IP);

  Serial.println("\n--- 3. Waiting for Client Connection... ---");
  while (WiFi.softAPgetStationNum() == 0) {
      Serial.print(".");
      delay(500);
  }
  Serial.println("\n✅ Client Connected! Starting Main Loop...");
}

// ==========================================
// --- LOOP (Optimized for Immediate Send) ---
// ==========================================

void loop() {
  // 1. Check Wi-Fi Client Status
  if (WiFi.softAPgetStationNum() == 0) {
      Serial.println("⚠️ Client disconnected! Waiting...");
      delay(1000);
      return; 
  }

  // 2. Ensure MQTT Connection
  if (!client.connected()) {
      reconnect();
  }
  client.loop(); // Important: Keep MQTT alive

  // 3. Check if SCD30 has NEW data (Non-blocking check)
  float co2 = 0, scd_temp = 0, scd_hum = 0;
  
  // This IF condition is key: it only enters if data is ready RIGHT NOW.
  if (scd30_read(scd30, co2, scd_temp, scd_hum)) {
      
      // Data is ready! Let's read the others immediately
      float bme_temp = 0, bme_hum = 0, pres = 0;
      float light = 0;
      
      if (!bme280_read(bme280, bme_temp, bme_hum, pres)) Serial.println("Warn: BME280 fail");
      if (!bh1750_read(bh1750, light)) Serial.println("Warn: BH1750 fail");
      
      // Fusion
      float avg_temp = (scd_temp + bme_temp) / 2.0;
      float avg_hum = (scd_hum + bme_hum) / 2.0;

      // Publish immediately
      publish_data(avg_temp, avg_hum, pres, co2, light);

      delay(10000);
      
      // No delay here! We go back to start of loop to wait for next data.
  }
  
  // Small delay to prevent CPU overheating while waiting (optional but good practice)
  delay(10); 
}