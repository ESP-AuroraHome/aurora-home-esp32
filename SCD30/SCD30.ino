#include <Wire.h>
#include "Adafruit_SCD30.h"

#ifndef SDA
#define SDA 21
#endif
#ifndef SCL
#define SCL 22
#endif

Adafruit_SCD30 scd30;

void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(SDA, SCL);
  Wire.setClock(50000);   // <= 100 kHz recommandé pour le SCD30
  Wire.setTimeOut(3000);  // pour clock stretching
  delay(2000);            // laisser le capteur booter

  if (!scd30.begin()) {
    Serial.println("[ERREUR] SCD30 introuvable @0x61.");
    while (1) delay(1000);
  }

  scd30.setMeasurementInterval(2); // toutes les 2 s
  Serial.println("SCD30 OK, lecture toutes ~2 s.");
}

void loop() {
  if (scd30.dataReady()) {
    if (scd30.read()) {
      Serial.printf("CO2=%.0f ppm  T=%.2f °C  H=%.1f %%\n",
                    scd30.CO2, scd30.temperature, scd30.relative_humidity);
    } else {
      Serial.println("[WARN] read() invalide");
    }
  }
  delay(200);
}