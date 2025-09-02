/*
  ESP32 – AuroraHome (Fusion capteurs + recal micro si dB < 0)
  Sortie unique par type : CO2, TEMP, HUM, PRESS, ALT, LUX, NOISE_DB

  Dépendances (Library Manager) :
   - Adafruit SCD30
   - BH1750 (Christopher Laws)
   - Adafruit BME280
   - Adafruit BMP280
   - Adafruit Unified Sensor
*/

#include <Arduino.h>
#include <Wire.h>

// ====== I2C PINS (ESP32) ======
#ifndef SDA
#define SDA 21
#endif
#ifndef SCL
#define SCL 22
#endif

// ====== SCD30 (Adafruit) ======
#include "Adafruit_SCD30.h"
Adafruit_SCD30 scd30;
bool hasSCD30 = false;

// ====== BH1750 (Christopher Laws) ======
#include <BH1750.h>
BH1750 lightMeter;
bool hasBH1750 = false;

// ====== BME280 / BMP280 (Adafruit) ======
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
Adafruit_BME280 bme;    // T/H/P
Adafruit_BMP280 bmp;    // T/P
enum EnvType { ENV_NONE, ENV_BME280, ENV_BMP280 };
EnvType envType = ENV_NONE;
bool hasEnv = false;
const uint8_t ENV_ADDRS[] = { 0x76, 0x77 };
float seaLevel_hPa = 1013.25f; // ajuste selon ta zone

// ====== Micro (HW-484) – analogique ======
const int   PIN_MIC              = 34;      // ADC1_CH6
const int   INIT_DELAY_MS        = 2000;
const int   THROWAWAY_READS      = 30;
const float VREF_MV              = 3300.0;
const int   SAMPLES_PER_WINDOW   = 250;
const int   US_BETWEEN_SAMPLES   = 200;     // ~5 kHz
const float MIC_EMA_ALPHA        = 0.25f;
const float DB_FLOOR             = -60.0f;

// — RÉ-ÉTALONNAGE —
// 1) règle "robuste" d'origine (très bas ET longtemps)
const float RECAL_TRIGGER_DB     = -12.0f;
const float RECAL_TRIGGER_ABS_MV = 0.20f;
const int   RECAL_TRIGGER_LOOPS  = 50;      // ~5 s

// 2) NOUVELLE RÈGLE demandée : si dB_rel < 0 → recalibrer après X boucles
const bool  RECAL_ON_NEGATIVE    = true;
const int   RECAL_NEG_LOOPS      = 20;      // ~2 s (20 x ~100 ms)

// Abandon calibration si bruit
const float RECAL_ABORT_DB_OVER  = +3.0f;

// Cooldown entre calibrations
const int   RECAL_COOLDOWN_LOOPS = 300;     // ~30 s

// Plancher math (éviter division par zéro)
const float MV_MIN               = 0.01f;

float mic_ema_rms_mv = 0.0f, mic_noise_floor_rms_mv = 1.0f;
enum MicMode { RUN, CALIB, COOLDOWN };
MicMode micMode = RUN;
int mic_lowCount = 0, mic_cooldownCount = 0, mic_calibCount = 0;
int mic_negCount = 0; // <— compteur pour la règle dB<0
double mic_calibAcc_rms_mv = 0.0;

// ====== FUSION & LISSAGE (EMA) ======
template<typename T> T clamp(T v, T lo, T hi){ return v<lo?lo:(v>hi?hi:v); }

struct Fused {
  float co2_ppm = NAN;    // SCD30
  float temp_C  = NAN;    // Fusion SCD30/BME/BMP
  float hum_pct = NAN;    // Fusion SCD30/BME
  float press_hPa = NAN;  // BME/BMP
  float alt_m   = NAN;    // dérivée de pression
  float lux     = NAN;    // BH1750
  float noise_db = NAN;   // Micro
};
const float EMA_ALPHA = 0.3f;
Fused fused, fusedEMA;

// ====== Timers ======
uint32_t tLux=0, tEnv=0, tSCD=0, tMic=0, tStatus=0;
const uint32_t PERIOD_LUX_MS    = 200;
const uint32_t PERIOD_ENV_MS    = 1000;
const uint32_t PERIOD_STATUS_MS = 1000;
const uint32_t PERIOD_MIC_MS    = 100;

// ====== MIC utils ======
uint16_t micReadADC(int pin){ return analogRead(pin); }
float micMeasureRMS_counts() {
  // Moyenne DC
  uint32_t acc=0;
  for(int i=0;i<SAMPLES_PER_WINDOW;i++){ acc+=micReadADC(PIN_MIC); delayMicroseconds(US_BETWEEN_SAMPLES); }
  const float mean_counts = acc / (float)SAMPLES_PER_WINDOW;
  // RMS AC
  double acc2=0;
  for(int i=0;i<SAMPLES_PER_WINDOW;i++){
    float x=micReadADC(PIN_MIC); float ac=x-mean_counts; acc2 += ac*ac;
    delayMicroseconds(US_BETWEEN_SAMPLES);
  }
  return sqrt(acc2 / SAMPLES_PER_WINDOW);
}
float micCountsToMilliVolts(float counts){ return counts * (VREF_MV / 4095.0f); }
void micStartCalibration(){
  micMode=CALIB; mic_calibCount=0; mic_calibAcc_rms_mv=0.0;
  Serial.println(F("[MIC] Ré-étalonnage bruit de fond… (silence ~1 s)"));
}
void micApplyCalibration(){
  mic_noise_floor_rms_mv = max(MV_MIN, (float)(mic_calibAcc_rms_mv / max(1, mic_calibCount)));
  mic_ema_rms_mv = mic_noise_floor_rms_mv;
  micMode = COOLDOWN; mic_cooldownCount = RECAL_COOLDOWN_LOOPS;
  mic_lowCount=0; mic_negCount=0;
  Serial.print(F("[MIC] Nouveau bruit de fond = "));
  Serial.print(mic_noise_floor_rms_mv, 3);
  Serial.println(F(" mV RMS"));
}
void micAbortCalibration(){
  Serial.println(F("[MIC] Bruit détecté, calibration annulée."));
  micMode = COOLDOWN; mic_cooldownCount = RECAL_COOLDOWN_LOOPS;
  mic_lowCount=0; mic_negCount=0;
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  delay(200);

  // I2C : réglages “cool” compatibles SCD30 (ok pour BH1750/BME/BMP)
  Wire.begin(SDA, SCL);
  Wire.setClock(50000);   // 50 kHz (tu peux tester 100 kHz si tout est stable)
  Wire.setTimeOut(3000);  // 3 s
  delay(2000);            // laisser booter les capteurs

  // SCD30
  hasSCD30 = scd30.begin();
  if (hasSCD30) {
    scd30.setMeasurementInterval(2);
    Serial.println(F("[OK] SCD30 @0x61"));
  } else {
    Serial.println(F("[WARN] SCD30 introuvable"));
  }

  // BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)
   || lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire)) {
    hasBH1750 = true; Serial.println(F("[OK] BH1750"));
  } else {
    Serial.println(F("[WARN] BH1750 introuvable"));
  }

  // BME/BMP280
  for (uint8_t addr : ENV_ADDRS) {
    if (bme.begin(addr)) { envType = ENV_BME280; hasEnv = true; break; }
    if (bmp.begin(addr)) { envType = ENV_BMP280; hasEnv = true; break; }
  }
  if (hasEnv) {
    if (envType == ENV_BME280) {
      bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                      Adafruit_BME280::SAMPLING_X2,
                      Adafruit_BME280::SAMPLING_X2,
                      Adafruit_BME280::SAMPLING_X4,
                      Adafruit_BME280::FILTER_X4,
                      Adafruit_BME280::STANDBY_MS_125);
      Serial.println(F("[OK] BME280"));
    } else {
      bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                      Adafruit_BMP280::SAMPLING_X2,
                      Adafruit_BMP280::SAMPLING_X4,
                      Adafruit_BMP280::FILTER_X4,
                      Adafruit_BMP280::STANDBY_MS_125);
      Serial.println(F("[OK] BMP280"));
    }
  } else {
    Serial.println(F("[WARN] Aucun BME280/BMP280"));
  }

  // MIC
  delay(INIT_DELAY_MS);
  analogSetPinAttenuation(PIN_MIC, ADC_11db);
  pinMode(PIN_MIC, INPUT);
  for (int i=0;i<THROWAWAY_READS;i++){ (void)analogRead(PIN_MIC); delay(10); }
  // calibration initiale ~1s
  double acc=0.0;
  for (int i=0;i<20;i++) acc += micCountsToMilliVolts(micMeasureRMS_counts());
  mic_noise_floor_rms_mv = max(MV_MIN, (float)(acc/20.0));
  mic_ema_rms_mv = mic_noise_floor_rms_mv;

  Serial.println(F("Init OK. Fusion multi-capteurs démarrée..."));
}

// ====== LOOP ======
void loop() {
  const uint32_t now = millis();

  // --- MIC (100 ms) ---
  if (now - tMic >= PERIOD_MIC_MS) {
    tMic = now;
    float rms_mv = micCountsToMilliVolts(micMeasureRMS_counts());
    mic_ema_rms_mv = MIC_EMA_ALPHA * rms_mv + (1.0f - MIC_EMA_ALPHA) * mic_ema_rms_mv;

    float denom = max(MV_MIN, mic_noise_floor_rms_mv);
    float ratio = max(mic_ema_rms_mv / denom, 1e-3f);
    float db_rel = 20.0f * log10f(ratio);
    if (db_rel < DB_FLOOR) db_rel = DB_FLOOR;

    // Sortie bruit
    fused.noise_db = db_rel;

    // === Déclenchement ré-étalonnage ===
    if (micMode == RUN) {
      // (a) règle robuste historique
      bool veryLow = (db_rel < RECAL_TRIGGER_DB) && (mic_ema_rms_mv < RECAL_TRIGGER_ABS_MV);
      if (veryLow) {
        if (++mic_lowCount >= RECAL_TRIGGER_LOOPS) micStartCalibration();
      } else {
        mic_lowCount = 0;
      }

      // (b) NOUVELLE règle : si dB_rel < 0 suffisamment longtemps
      if (RECAL_ON_NEGATIVE) {
        if (db_rel < 0.0f) {
          if (++mic_negCount >= RECAL_NEG_LOOPS) {
            micStartCalibration();
            mic_negCount = 0; // reset pour ne pas relancer immédiatement
          }
        } else {
          mic_negCount = 0;
        }
      }
    }
    else if (micMode == CALIB) {
      if (db_rel > RECAL_ABORT_DB_OVER) {
        micAbortCalibration();
      } else {
        mic_calibAcc_rms_mv += rms_mv;
        if (++mic_calibCount >= 20) micApplyCalibration(); // ~1 s de silence
      }
    }
    else if (micMode == COOLDOWN) {
      if (--mic_cooldownCount <= 0) { micMode = RUN; mic_lowCount = 0; mic_negCount = 0; }
    }
  }

  // --- BH1750 (200 ms) ---
  if (hasBH1750 && (now - tLux >= PERIOD_LUX_MS)) {
    tLux = now;
    float lux = lightMeter.readLightLevel();
    if (lux >= 0) fused.lux = lux;
  }

  // --- ENV (BME/BMP) 1 s ---
  if (hasEnv && (now - tEnv >= PERIOD_ENV_MS)) {
    tEnv = now;
    float T = NAN, H = NAN, P = NAN;
    if (envType == ENV_BME280) {
      T = bme.readTemperature();
      H = bme.readHumidity();
      P = bme.readPressure() / 100.0f; // hPa
    } else if (envType == ENV_BMP280) {
      T = bmp.readTemperature();
      P = bmp.readPressure() / 100.0f;
    }
    if (!isnan(T)) fused.temp_C = isnan(fused.temp_C) ? T : (T + fused.temp_C) / 2.0f; // provisoire
    if (!isnan(H)) fused.hum_pct = H;
    if (!isnan(P)) {
      fused.press_hPa = P;
      fused.alt_m = 44330.0f * (1.0f - pow(fused.press_hPa / seaLevel_hPa, 0.1903f));
    }
  }

  // --- SCD30 (2 s) ---
  if (hasSCD30 && scd30.dataReady()) {
    if (scd30.read()) {
      fused.co2_ppm = scd30.CO2;
      // fusion T/H finale se fait à tStatus pour éviter de relire trop souvent
    }
  }

  // --- FUSION FINALE + AFFICHAGE (1 s) ---
  if (now - tStatus >= PERIOD_STATUS_MS) {
    tStatus = now;

    // Candidats T/H : ENV + SCD30
    float T_env = NAN, H_env = NAN;
    float T_scd = NAN, H_scd = NAN;

    if (hasEnv) {
      if (envType == ENV_BME280) {
        T_env = bme.readTemperature();
        H_env = bme.readHumidity();
      } else if (envType == ENV_BMP280) {
        T_env = bmp.readTemperature();
      }
    }
    if (hasSCD30) {
      T_scd = scd30.temperature;
      H_scd = scd30.relative_humidity;
    }

    // TEMP fusion
    float T_fused = NAN;
    int nt=0; float Tcand[2];
    if (!isnan(T_env)) Tcand[nt++] = T_env;
    if (!isnan(T_scd)) Tcand[nt++] = T_scd;
    if (nt==2) {
      float diff = fabs(Tcand[0]-Tcand[1]);
      if (diff > 3.0f) {
        float ref = isnan(fusedEMA.temp_C) ? (Tcand[0]+Tcand[1])/2.0f : fusedEMA.temp_C;
        T_fused = (fabs(Tcand[0]-ref) < fabs(Tcand[1]-ref)) ? Tcand[0] : Tcand[1];
      } else T_fused = (Tcand[0]+Tcand[1])*0.5f;
    } else if (nt==1) T_fused = Tcand[0];

    // HUM fusion
    float H_fused = NAN;
    int nh=0; float Hcand[2];
    if (!isnan(H_env)) Hcand[nh++] = H_env;
    if (!isnan(H_scd)) Hcand[nh++] = H_scd;
    if (nh==2) {
      float diff = fabs(Hcand[0]-Hcand[1]);
      if (diff > 15.0f) {
        float ref = isnan(fusedEMA.hum_pct) ? (Hcand[0]+Hcand[1])/2.0f : fusedEMA.hum_pct;
        H_fused = (fabs(Hcand[0]-ref) < fabs(Hcand[1]-ref)) ? Hcand[0] : Hcand[1];
      } else H_fused = (Hcand[0]+Hcand[1])*0.5f;
    } else if (nh==1) H_fused = Hcand[0];

    // EMA global
    auto emaUpdate = [](float cur, float prev)->float {
      if (isnan(cur)) return prev;
      if (isnan(prev)) return cur;
      return EMA_ALPHA*cur + (1.0f-EMA_ALPHA)*prev;
    };

    fusedEMA.co2_ppm  = emaUpdate(fused.co2_ppm,  fusedEMA.co2_ppm);
    fusedEMA.temp_C   = emaUpdate(T_fused,        fusedEMA.temp_C);
    fusedEMA.hum_pct  = emaUpdate(H_fused,        fusedEMA.hum_pct);
    fusedEMA.press_hPa= emaUpdate(fused.press_hPa,fusedEMA.press_hPa);
    fusedEMA.alt_m    = emaUpdate(fused.alt_m,    fusedEMA.alt_m);
    fusedEMA.lux      = emaUpdate(fused.lux,      fusedEMA.lux);
    fusedEMA.noise_db = emaUpdate(fused.noise_db, fusedEMA.noise_db);

    // Une seule ligne par type
    Serial.print(F("CO2="));      if (isnan(fusedEMA.co2_ppm))  Serial.print(F("NA")); else Serial.print(fusedEMA.co2_ppm, 0);
    Serial.print(F(" ppm | TEMP=")); if (isnan(fusedEMA.temp_C)) Serial.print(F("NA")); else Serial.print(fusedEMA.temp_C, 2);
    Serial.print(F(" °C | HUM="));   if (isnan(fusedEMA.hum_pct)) Serial.print(F("NA")); else Serial.print(fusedEMA.hum_pct, 1);
    Serial.print(F(" % | PRESS="));  if (isnan(fusedEMA.press_hPa)) Serial.print(F("NA")); else Serial.print(fusedEMA.press_hPa, 1);
    Serial.print(F(" hPa | ALT≈")); if (isnan(fusedEMA.alt_m)) Serial.print(F("NA")); else Serial.print(fusedEMA.alt_m, 1);
    Serial.print(F(" m | LUX="));   if (isnan(fusedEMA.lux)) Serial.print(F("NA")); else Serial.print(fusedEMA.lux, 1);
    Serial.print(F(" lx | NOISE_DB=")); if (isnan(fusedEMA.noise_db)) Serial.print(F("NA")); else Serial.print(fusedEMA.noise_db, 1);
    if (micMode == CALIB)    Serial.print(F(" [CALIB]"));
    if (micMode == COOLDOWN) Serial.print(F(" [COOLDOWN]"));
    Serial.println();
  }
}