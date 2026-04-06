#include "battery.h"

// --------- CONFIG ----------
static const int PIN_BAT = 18;

// Divider: Battery+ -> R_TOP -> ADC -> R_BOT -> GND
static const float R_TOP = 270000.0f;   // 270k
static const float R_BOT = 30000.0f;    // 30k
static const float DIV_GAIN = (R_TOP + R_BOT) / R_BOT; // = 10.0

// Sampling
static const int   SAMPLES = 16;        // increase to reduce noise
static const int   SAMPLE_DELAY_US = 250;

// Simple IIR smoothing (0..1). Higher = smoother/slower.
static const float ALPHA = 0.15f;
static float g_vbat_filt = 0.0f;
static bool  g_has_filt = false;

void battery_init() {
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_BAT, ADC_11db);

  // throw away a few reads to settle ADC
  for (int i = 0; i < 8; i++) {
    (void)analogRead(PIN_BAT);
    delay(2);
  }
}

static float read_adc_pin_volts_avg() {
  // Using analogReadMilliVolts() is usually more accurate than raw ADC.
  // It uses calibration data when available.
  uint32_t mv_sum = 0;

  for (int i = 0; i < SAMPLES; i++) {
    mv_sum += (uint32_t)analogReadMilliVolts(PIN_BAT);
    delayMicroseconds(SAMPLE_DELAY_US);
  }

  float mv = (float)mv_sum / (float)SAMPLES;
  return mv / 1000.0f; // ADC pin volts
}

float battery_read_volts() {
  float vadc = read_adc_pin_volts_avg();
  float vbat = vadc * DIV_GAIN;

  // Filter
  if (!g_has_filt) {
    g_vbat_filt = vbat;
    g_has_filt = true;
  } else {
    g_vbat_filt = (ALPHA * vbat) + ((1.0f - ALPHA) * g_vbat_filt);
  }

  return g_vbat_filt;
}

// Optional: very rough lead-acid % estimate (best at rest, not under load).
uint8_t battery_percent_12v(float v) {
  // Clamp expected range
  if (v >= 12.70f) return 100;
  if (v >= 12.50f) return 85;
  if (v >= 12.30f) return 60;
  if (v >= 12.10f) return 35;
  if (v >= 11.90f) return 15;
  return 5;
}
