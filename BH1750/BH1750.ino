/*
  ESP32 + BH1750 (Christopher Laws)
  - I2C: SDA=21, SCL=22
  - Essaie 0x23 puis 0x5C
  - Mode continu haute résolution
*/

#include <Wire.h>
#include <BH1750.h>

#ifndef SDA
#define SDA 21
#endif
#ifndef SCL
#define SCL 22
#endif

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(SDA, SCL);
  Wire.setClock(400000); // 400 kHz

  // Tente d'abord 0x23
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
    Serial.println(F("[OK] BH1750 @0x23, mode CONTINUOUS_HIGH_RES_MODE"));
  }
  // Sinon tente 0x5C
  else if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire)) {
    Serial.println(F("[OK] BH1750 @0x5C, mode CONTINUOUS_HIGH_RES_MODE"));
  }
  else {
    Serial.println(F("[ERREUR] BH1750 introuvable (0x23/0x5C). Vérifie VCC(3V3), GND, SDA=21, SCL=22."));
    while (true) delay(1000);
  }

  // petit temps pour la 1re mesure
  delay(180);
}

void loop() {
  float lux = lightMeter.readLightLevel(); // renvoie -1 si erreur

  if (lux < 0) {
    Serial.println(F("[WARN] Lecture invalide"));
  } else {
    Serial.print(F("Lux="));
    Serial.println(lux, 1);
  }

  delay(200);
}