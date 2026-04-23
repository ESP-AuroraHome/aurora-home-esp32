/* AuroraHome – Micro HW-484 sur ESP32
   Lecture analogique stable + auto-ré-étalonnage robuste (anti-boucle)

   câblage:
     HW-484 +  -> 3V3 ESP32
     HW-484 G  -> GND
     HW-484 A0 -> GPIO34 (ADC1)
*/

#include <Arduino.h>

// ====== Réglages ======
const int   PIN_MIC              = 34;      // GPIO34 = ADC1_CH6
const int   INIT_DELAY_MS        = 2000;    // stabilisation module + ADC
const int   THROWAWAY_READS      = 30;      // lectures jetées au début
const float VREF_MV              = 3300.0;  // pleine échelle avec 11 dB (approx)

// Fenêtre RMS ~50 ms @ ~5 kHz => ~250 échantillons
const int   SAMPLES_PER_WINDOW   = 250;
const int   US_BETWEEN_SAMPLES   = 200;     // ~5 kHz

// Lissage
const float EMA_ALPHA            = 0.25f;   // 0..1
const float DB_FLOOR             = -60.0f;  // affichage

// Déclenchement ré-étalonnage (hystérésis + absolu)
const float RECAL_TRIGGER_DB     = -12.0f;  // dB en-dessous du plancher
const float RECAL_TRIGGER_ABS_MV = 0.20f;   // ET niveau absolu très bas
const int   RECAL_TRIGGER_LOOPS  = 50;      // ~5 s (50 x ~100 ms)

// Abandon calibration si bruit
const float RECAL_ABORT_DB_OVER  = +3.0f;   // bruit > +3 dB => abort

// Cooldown entre calibrations
const int   RECAL_COOLDOWN_LOOPS = 300;     // ~30 s

// Planchers math (éviter division zéro)
const float MV_MIN               = 0.01f;   // 10 µV

// ====== State ======
float ema_rms_mv = 0.0f;
float noise_floor_rms_mv = 1.0f;
enum Mode { RUN, CALIB, COOLDOWN };
Mode mode = RUN;

int lowCount = 0;                // boucles consécutives sous le seuil
int cooldownCount = 0;           // compte à rebours cooldown

int  calibCount = 0;
double calibAcc_rms_mv = 0.0;

// ====== Utils ======
uint16_t readADC(int pin) { return analogRead(pin); }

float measureRMS_counts() {
  // 1) moyenne DC
  uint32_t acc = 0;
  for (int i = 0; i < SAMPLES_PER_WINDOW; i++) {
    acc += readADC(PIN_MIC);
    delayMicroseconds(US_BETWEEN_SAMPLES);
  }
  const float mean_counts = acc / (float)SAMPLES_PER_WINDOW;

  // 2) RMS autour de la moyenne
  double acc2 = 0;
  for (int i = 0; i < SAMPLES_PER_WINDOW; i++) {
    float x = readADC(PIN_MIC);
    float ac = x - mean_counts;
    acc2 += ac * ac;
    delayMicroseconds(US_BETWEEN_SAMPLES);
  }
  return sqrt(acc2 / SAMPLES_PER_WINDOW);
}

float countsToMilliVolts(float counts) {
  const float mv_per_count = VREF_MV / 4095.0f;
  return counts * mv_per_count;
}

void throwAwayReads(int n, int delay_ms) {
  for (int i = 0; i < n; i++) { (void)readADC(PIN_MIC); delay(delay_ms); }
}

void startCalibration() {
  mode = CALIB;
  calibCount = 0;
  calibAcc_rms_mv = 0.0;
  Serial.println(F("[INFO] Ré-étalonnage bruit de fond… (silence ~1 s)"));
}

void applyCalibration() {
  // Pas de plancher artificiel à 1.0 mV : on accepte les SILENCES très bas
  noise_floor_rms_mv = max(MV_MIN, (float)(calibAcc_rms_mv / max(1, calibCount)));
  ema_rms_mv = noise_floor_rms_mv; // recentrer l’EMA
  mode = COOLDOWN;
  cooldownCount = RECAL_COOLDOWN_LOOPS;
  lowCount = 0;

  Serial.print(F("[INFO] Nouveau bruit de fond = "));
  Serial.print(noise_floor_rms_mv, 3);
  Serial.println(F(" mV RMS"));
}

void abortCalibration() {
  Serial.println(F("[WARN] Bruit détecté, calibration annulée."));
  mode = COOLDOWN;              // même si abort, on part en cooldown pour éviter la boucle
  cooldownCount = RECAL_COOLDOWN_LOOPS;
  lowCount = 0;
}

void setup() {
  Serial.begin(115200);
  delay(50);

  delay(INIT_DELAY_MS);
  analogSetPinAttenuation(PIN_MIC, ADC_11db);
  pinMode(PIN_MIC, INPUT);
  throwAwayReads(THROWAWAY_READS, 10);

  // Calibration initiale ~1 s
  double acc_rms_mv = 0.0;
  for (int i=0; i<20; i++) {
    acc_rms_mv += countsToMilliVolts(measureRMS_counts());
  }
  noise_floor_rms_mv = max(MV_MIN, (float)(acc_rms_mv / 20.0));
  ema_rms_mv = noise_floor_rms_mv;

  Serial.println(F("Init OK. Mesure stable démarrée..."));
}

void loop() {
  // Mesure fenêtre (~100 ms)
  float rms_mv = countsToMilliVolts( measureRMS_counts() );
  ema_rms_mv = EMA_ALPHA * rms_mv + (1.0f - EMA_ALPHA) * ema_rms_mv;

  // dB relatifs (protégés)
  float denom = max(MV_MIN, noise_floor_rms_mv);
  float ratio = max(ema_rms_mv / denom, 1e-3f);
  float db_rel = 20.0f * log10f(ratio);
  if (db_rel < DB_FLOOR) db_rel = DB_FLOOR;

  // Affichage
  int bars = (int)constrain(map((long)(db_rel*10), (long)(-600), (long)(200), 0, 40), 0, 40);
  Serial.print(F("RMS="));
  Serial.print(ema_rms_mv, 3);
  Serial.print(F(" mV | dB_rel="));
  Serial.print(db_rel, 1);
  Serial.print(F(" | "));
  for (int i=0;i<bars;i++) Serial.print('#');
  if (mode == CALIB) Serial.print(F("  [CALIB]"));
  if (mode == COOLDOWN) Serial.print(F("  [COOLDOWN]"));
  Serial.println();

  // Gestion des états
  if (mode == RUN) {
    // Déclenchement SEULEMENT si très bas et longtemps
    bool veryLow = (db_rel < RECAL_TRIGGER_DB) && (ema_rms_mv < RECAL_TRIGGER_ABS_MV);
    if (veryLow) {
      if (++lowCount >= RECAL_TRIGGER_LOOPS) {
        startCalibration();
      }
    } else {
      lowCount = 0;
    }
  }
  else if (mode == CALIB) {
    // Pendant calibration : abandon si bruit > +3 dB
    if (db_rel > RECAL_ABORT_DB_OVER) {
      abortCalibration();
    } else {
      calibAcc_rms_mv += rms_mv;
      if (++calibCount >= 20) {  // ~1 s de silence suffisant
        applyCalibration();
      }
    }
  }
  else if (mode == COOLDOWN) {
    if (--cooldownCount <= 0) {
      mode = RUN;
      lowCount = 0;
    }
  }
}
