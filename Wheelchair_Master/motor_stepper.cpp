#include "motor_stepper.h"
#include "pins_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

// =====================
// USER TUNING
// =====================
// Speeds in pulses-per-second (PPS). Start low, increase slowly.
static const uint32_t FAST_PPS = 350;
static const uint32_t SLOW_PPS = 180;

// Ramp tuning
static const uint32_t RAMP_STEP_PPS = 25;                 // PPS change per update
static const TickType_t RAMP_PERIOD = pdMS_TO_TICKS(20);  // update every 20ms

// Direction inversion to match motor mounting / wiring
// motorIndex 0 = M1 = LEFT
// motorIndex 1 = M2 = RIGHT
static const bool INVERT_M1_DIR = true;   // <-- IMPORTANT for your current issue
static const bool INVERT_M2_DIR = false;

// =====================
// INTERNAL STATE
// =====================
static volatile bool g_enabled = false;

// current and target PPS
static volatile uint32_t curPpsL = 0, curPpsR = 0;
static volatile uint32_t tgtPpsL = 0, tgtPpsR = 0;

// direction
static volatile bool dirL = true, dirR = true;

// =====================
// LOW LEVEL HELPERS
// =====================
static inline void set_en_pins(bool enable) {
  // EN Active Low
  digitalWrite(PIN_M1_EN, enable ? LOW : HIGH);
  digitalWrite(PIN_M2_EN, enable ? LOW : HIGH);
}

static inline void set_ledc_pps(uint32_t left, uint32_t right) {
  // hz = 0 stops output
  ledcWriteTone(PIN_M1_PUL, left);
  ledcWriteTone(PIN_M2_PUL, right);
}

static inline void apply_dir_pins() {
  // Convert forward bool -> logic level
  bool l = dirL ? HIGH : LOW;
  bool r = dirR ? HIGH : LOW;

  // Per motor inversion
  if (INVERT_M1_DIR) l = !l;
  if (INVERT_M2_DIR) r = !r;

  digitalWrite(PIN_M1_DIR, l);
  digitalWrite(PIN_M2_DIR, r);
}

// =====================
// RAMP TASK
// =====================
static void ramp_task(void* pv) {
  (void)pv;

  while (true) {
    if (!g_enabled) {
      // Hard stop when disabled
      curPpsL = curPpsR = 0;
      tgtPpsL = tgtPpsR = 0;
      set_ledc_pps(0, 0);
      vTaskDelay(RAMP_PERIOD);
      continue;
    }

    // Apply direction each cycle
    apply_dir_pins();

    // Ramp left
    uint32_t cL = curPpsL, tL = tgtPpsL;
    if (cL < tL) {
      uint32_t n = cL + RAMP_STEP_PPS;
      curPpsL = (n > tL) ? tL : n;
    } else if (cL > tL) {
      uint32_t n = (cL > RAMP_STEP_PPS) ? (cL - RAMP_STEP_PPS) : 0;
      curPpsL = (n < tL) ? tL : n;
    }

    // Ramp right
    uint32_t cR = curPpsR, tR = tgtPpsR;
    if (cR < tR) {
      uint32_t n = cR + RAMP_STEP_PPS;
      curPpsR = (n > tR) ? tR : n;
    } else if (cR > tR) {
      uint32_t n = (cR > RAMP_STEP_PPS) ? (cR - RAMP_STEP_PPS) : 0;
      curPpsR = (n < tR) ? tR : n;
    }

    set_ledc_pps(curPpsL, curPpsR);
    vTaskDelay(RAMP_PERIOD);
  }
}

// =====================
// PUBLIC API
// =====================
void motor_init_safe() {
  // Pin modes
  pinMode(PIN_M1_EN, OUTPUT);
  pinMode(PIN_M1_DIR, OUTPUT);
  pinMode(PIN_M1_PUL, OUTPUT);

  pinMode(PIN_M2_EN, OUTPUT);
  pinMode(PIN_M2_DIR, OUTPUT);
  pinMode(PIN_M2_PUL, OUTPUT);

  // Boot safe: motors disabled, no pulses
  g_enabled = false;
  set_en_pins(false);

  dirL = true;
  dirR = true;
  apply_dir_pins();

  // LEDC attach pulse pins
  // Arduino ESP32 v3.x: ledcAttach(pin, freq, resolution_bits)
  ledcAttach(PIN_M1_PUL, 1000, 8);
  ledcAttach(PIN_M2_PUL, 1000, 8);
  set_ledc_pps(0, 0);

  // Start ramp task on core 1
  xTaskCreatePinnedToCore(
    ramp_task,
    "MotorRamp",
    4096,
    nullptr,
    5,
    nullptr,
    1
  );
}

void motor_enable(bool enable) {
  if (!enable) {
    // stop pulses first
    tgtPpsL = tgtPpsR = 0;
    curPpsL = curPpsR = 0;
    set_ledc_pps(0, 0);
  }

  g_enabled = enable;
  set_en_pins(enable);
}

bool motor_is_enabled() {
  return g_enabled;
}

void motor_apply_command(motion_cmd_t cmd) {
  if (!g_enabled) {
    tgtPpsL = tgtPpsR = 0;
    return;
  }

  switch (cmd) {
    case CMD_F:
      dirL = true;  dirR = true;
      tgtPpsL = FAST_PPS; tgtPpsR = FAST_PPS;
      break;

    case CMD_B:
      dirL = false; dirR = false;
      tgtPpsL = FAST_PPS; tgtPpsR = FAST_PPS;
      break;

    case CMD_LH:
      dirL = false; dirR = true;
      tgtPpsL = FAST_PPS; tgtPpsR = FAST_PPS;
      break;

    case CMD_RH:
      dirL = true;  dirR = false;
      tgtPpsL = FAST_PPS; tgtPpsR = FAST_PPS;
      break;

    case CMD_LF:
      dirL = true;  dirR = true;
      tgtPpsL = SLOW_PPS; tgtPpsR = FAST_PPS;
      break;

    case CMD_RF:
      dirL = true;  dirR = true;
      tgtPpsL = FAST_PPS; tgtPpsR = SLOW_PPS;
      break;

    case CMD_LB:
      dirL = false; dirR = false;
      tgtPpsL = SLOW_PPS; tgtPpsR = FAST_PPS;
      break;

    case CMD_RB:
      dirL = false; dirR = false;
      tgtPpsL = FAST_PPS; tgtPpsR = SLOW_PPS;
      break;

    case CMD_STOP:
    default:
      tgtPpsL = tgtPpsR = 0;
      break;
  }
}
