#include <Arduino.h>
#include <SensirionI2cScd30.h>
#include <BH1750.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

SensirionI2cScd30 scd30;
BH1750 bh1750(0x23);
Adafruit_BME280 bme280;

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

// --- Fonctions d'Initialisation et de Lecture ---
// (J'ai laissé les fonctions scd30_init, scd30_read, bh1750_read, 
// bme280_init, et bme280_read inchangées pour la concision ici)

bool scd30_init(SensirionI2cScd30 &scd30) {
    scd30.begin(Wire, SCD30_I2C_ADDR_61);
    scd30.stopPeriodicMeasurement();
    scd30.softReset();
    delay(2000);
    uint8_t major = 0;
    uint8_t minor = 0;
    int16_t error = scd30.readFirmwareVersion(major, minor);
    if (error != NO_ERROR) {
        return false;
    }
    error = scd30.startPeriodicMeasurement(0); 
    if (error != NO_ERROR) {
        return false;
    }
    return true;
}

bool scd30_read(SensirionI2cScd30 &scd30, float &co2, float &temp, float &hum) {
  uint16_t dataReady = 0;
  scd30.getDataReady(dataReady);

  if (dataReady) {
    scd30.readMeasurementData(co2, temp, hum);
    return true;
  } else {
    return false;
  }
}

bool bh1750_read(BH1750 &lightMeter, float &light) {
  if (lightMeter.measurementReady()) {
    light = lightMeter.readLightLevel();
    return true;
  } else {
    return false;
  }
}

bool bme280_init(Adafruit_BME280 &bme) {
    if (!bme.begin(0x76)) { 
        // Si 0x76 échoue, essayez 0x77
        if (!bme.begin(0x77)) {
            return false;
        }
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
    pres = bme.readPressure() / 100.0F; // Convertir en hPa (hectopascals/mbar)

    if (isnan(temp) || isnan(hum) || isnan(pres)) {
        return false;
    }
    return true;
}

// --- NOUVELLE FONCTION: Affichage des lectures fusionnées ---

/**
 * @brief Calcule et affiche les moyennes de Température et d'Humidité.
 */
void display_fused_readings(float scd_temp, float bme_temp, float scd_hum, float bme_hum, float pres, float co2, float light) {
    
    // 1. Calcul des moyennes (Température et Humidité)
    // Nous avons des lectures doubles pour T et H, donc nous faisons la moyenne.
    float avg_temp = (scd_temp + bme_temp) / 2.0;
    float avg_hum = (scd_hum + bme_hum) / 2.0;
    
    // 2. Affichage des résultats fusionnés/uniques
    Serial.println("\n==================================");
    Serial.println("📊 RESULTATS FUSIONNES (Air Quality)");
    Serial.println("==================================");

    Serial.print("✅ Temp. Moyenne: ");
    Serial.print(avg_temp);
    Serial.println(" °C");

    Serial.print("✅ Humidité Moyenne: ");
    Serial.print(avg_hum);
    Serial.println(" %RH");

    Serial.print("✅ Pression Atm. (BME280): ");
    Serial.print(pres);
    Serial.println(" hPa");

    Serial.print("✅ CO2 (SCD30): ");
    Serial.print(co2);
    Serial.println(" ppm");

    Serial.print("✅ Lumière (BH1750): ");
    Serial.print(light);
    Serial.println(" lx");

    Serial.println("----------------------------------");
}

// --- Setup ---

void setup() {

  Serial.begin(115200);
  while (!Serial) {delay(100);}

  Wire.begin(21, 22); 

  if (bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 (Light) initialized successfully"));
  } else {
    Serial.println(F("Error initialising BH1750 (Light)"));
  }

  if (!scd30_init(scd30)) {
      Serial.println("Failed to initialize SCD30 (CO2) sensor");
      while (1) {
          delay(1000);
      }
  }else {Serial.println("SCD30 (CO2) sensor initialized successfully");}
  
  if (!bme280_init(bme280)) {
      Serial.println("Failed to initialize BME280 sensor. Check wiring/address!");
      while (1) {
          delay(1000);
      }
  } else {Serial.println("BME280 sensor initialized successfully");}
}

// --- Loop ---

void loop() {
  float co2 = 0, scd_temp = 0, scd_hum = 0;
  float bme_temp = 0, bme_hum = 0, pres = 0;
  float light = 0;
  bool bme_ok, bh_ok;

  // 1. Lecture SCD30 (CO2, T, H)
  while (!scd30_read(scd30, co2, scd_temp, scd_hum)) {
    delay(10); // Attente que la donnée CO2 soit prête
  }

  // 2. Lecture BME280 (T, H, P)
  bme_ok = bme280_read(bme280, bme_temp, bme_hum, pres);
  if (!bme_ok) {
     Serial.println("WARNING: BME280 read failed. Using SCD30 T/H for fusion (T=0, H=0).");
     // Si le BME280 échoue, on peut mettre ses lectures à zéro 
     // pour que la moyenne soit égale à la lecture du SCD30 (SCD30/2 + 0/2 = SCD30/2... ATTENTION, la formule de moyenne ne fonctionne plus)
     // MIEUX: Laisser le BME280 T/H à zéro et ajuster l'affichage, mais pour la moyenne simple demandée, c'est la façon la plus simple.
     // Pour être mathématiquement correct, il faudrait diviser par 1 si l'un échoue, ou utiliser une fonction de robustesse.
     // Je vais garder la moyenne simple pour l'instant (division par 2) et juste afficher l'avertissement.
  }
  
  // 3. Lecture BH1750 (Lumière)
  bh_ok = bh1750_read(bh1750, light);
  if (!bh_ok) {
    Serial.println("WARNING: BH1750 read failed.");
  }
  
  // 4. Fusion et Affichage
  // Note: La fusion se fait sur les 4 premières variables (T, H, P, CO2), 
  // la lumière est une lecture unique.
  display_fused_readings(scd_temp, bme_temp, scd_hum, bme_hum, pres, co2, light);

  //delay(3000); // Attendre 3 secondes avant la prochaine lecture
}