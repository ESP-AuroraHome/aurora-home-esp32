/*
  ESP32 + GY-BME280 / GY-BMP280 (I2C) – Auto-détection et lecture stable
  - Détecte BME280 (Temp/Hum/Pression) ou BMP280 (Temp/Pression)
  - Tente 0x76 puis 0x77
  - Affiche T (°C), P (hPa), H (%) si dispo, Altitude estimée (m)

  Câblage (ESP32):
    3V3 -> VIN/3V3  |  GND -> GND
    SDA (GPIO21) -> SDA
    SCL (GPIO22) -> SCL
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>

// Choix des broches I2C ESP32 (par défaut : 21/22)
#ifndef SDA
#define SDA 21
#endif
#ifndef SCL
#define SCL 22
#endif

// Deux adresses possibles
const uint8_t ADDRS[] = { 0x76, 0x77 };

// Capteurs (on en utilisera un seul)
Adafruit_BME280 bme;   // T/H/P
Adafruit_BMP280 bmp;   // T/P

enum SensorType { NONE, IS_BME280, IS_BMP280 };
SensorType sensorType = NONE;

// Réglages capteur (sur-/sous-échantillonnage)
void configureBME() {
  // Oversampling “confort” pour lecture stable
  bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                  Adafruit_BME280::SAMPLING_X2,   // Temp
                  Adafruit_BME280::SAMPLING_X2,   // Hum
                  Adafruit_BME280::SAMPLING_X4,   // Press
                  Adafruit_BME280::FILTER_X4,
                  Adafruit_BME280::STANDBY_MS_125);
}

void configureBMP() {
  // Pour BMP280 (pas d’humidité)
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,   // Temp
                  Adafruit_BMP280::SAMPLING_X4,   // Press
                  Adafruit_BMP280::FILTER_X4,
                  Adafruit_BMP280::STANDBY_MS_125);
}

// Pression au niveau de la mer (hPa) ~ Rennes ≈ 1013.25 par défaut
// Ajuste si tu connais la pression locale pour une altitude plus juste
float seaLevel_hPa = 1013.25f;

// Essaye d’initialiser BME280 ou BMP280 sur addr donnée
bool tryInitAt(uint8_t addr) {
  if (bme.begin(addr)) {
    sensorType = IS_BME280;
    configureBME();
    Serial.print(F("[OK] BME280 trouvé à 0x"));
    Serial.println(addr, HEX);
    return true;
  }
  if (bmp.begin(addr)) {
    sensorType = IS_BMP280;
    configureBMP();
    Serial.print(F("[OK] BMP280 trouvé à 0x"));
    Serial.println(addr, HEX);
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // I2C init
  Wire.begin(SDA, SCL);
  Wire.setClock(400000); // I2C rapide (400 kHz)

  // Petit délai pour la stabilité (alimentation, régulateur, capteur)
  delay(200);

  // Tentative d’init sur 0x76 puis 0x77
  for (uint8_t addr : ADDRS) {
    if (tryInitAt(addr)) break;
  }

  if (sensorType == NONE) {
    Serial.println(F("[ERREUR] Aucun BME280/BMP280 détecté (0x76/0x77)."));
    Serial.println(F(" - Vérifie SDA/SCL, l’alim 3V3, la masse commune."));
    Serial.println(F(" - Essaie de relier l’autre adresse (pont sur la carte)."));
    while (true) { delay(1000); }
  }

  Serial.println(F("Init OK. Lecture toutes les 1 s...\n"));
}

void loop() {
  float T = NAN, H = NAN, P_hPa = NAN, altitude_m = NAN;

  bool valid = true;
  if (sensorType == IS_BME280) {
    T     = bme.readTemperature();            // °C
    P_hPa = bme.readPressure() / 100.0f;      // Pa -> hPa
    H     = bme.readHumidity();               // %
  } else if (sensorType == IS_BMP280) {
    T     = bmp.readTemperature();
    P_hPa = bmp.readPressure() / 100.0f;
  } else {
    valid = false;
  }

  // Sanity checks simples
  if (isnan(T) || isnan(P_hPa) || (sensorType == IS_BME280 && isnan(H))) {
    valid = false;
  }

  if (!valid) {
    Serial.println(F("[WARN] Lecture invalide, on réessaie..."));
    delay(500);
    return;
  }

  // Altitude estimée (formule barométrique simple)
  // altitude = 44330 * (1 - (P / P0)^(1/5.255))
  altitude_m = 44330.0f * (1.0f - pow(P_hPa / seaLevel_hPa, 0.1903f));

  // Affichage propre
  Serial.print(F("T=")); Serial.print(T, 2); Serial.print(F(" °C  "));
  if (sensorType == IS_BME280) {
    Serial.print(F("H=")); Serial.print(H, 1); Serial.print(F(" %  "));
  }
  Serial.print(F("P=")); Serial.print(P_hPa, 2); Serial.print(F(" hPa  "));
  Serial.print(F("Alt≈")); Serial.print(altitude_m, 1); Serial.println(F(" m"));

  delay(1000);
}