#include <Arduino.h>
#include <scd30.hpp>
#include <bh1750.hpp>

#include <Wire.h>


void setup() {

  Serial.begin(115200);
  while (!Serial) {delay(100);}

  Wire.begin(21, 22);

  if (!scd30_init(sensor)) {
      Serial.println("Failed to initialize SCD30 sensor");
      while (1) {
          delay(1000);
      }
  }else {Serial.println("SCD30 sensor initialized successfully");}


}

void loop() {
  float co2, temp, hum;
  int result = scd30_read(co2, temp, hum);
  if (result == 0) {
    Serial.print("CO2: ");
    Serial.print(co2);
    Serial.print(" ppm, Temp: ");
    Serial.print(temp);
    Serial.print(" °C, Humidity: ");
    Serial.print(hum);
    Serial.println(" %RH");
  } else {Serial.println("Error reading from SCD30 sensor");}
}

/*
P.O.C.



tout faire dans le main et laisser copilot le répartir dans des .cpp extern


*/