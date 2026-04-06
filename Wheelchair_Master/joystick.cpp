#include "joystick.h"
#include <Arduino.h>
#include "pins_master.h"
#include "control_cmd.h"

// Boot-calibrated center
static int centerX = 2048;
static int centerY = 2048;

// Filtered values
static int filtX = 2048;
static int filtY = 2048;

// Tune these
static const int DEAD = 380;   // dead zone
static const int TURN = 650;   // threshold for LF/RF/LB/RB

static int read_adc_avg(int pin) {
  long s = 0;
  for (int i = 0; i < 5; i++) {
    s += analogRead(pin);
    delayMicroseconds(300);
  }
  return (int)(s / 5);
}

void joystick_calibrate(uint16_t samples) {
  long sx = 0, sy = 0;
  for (uint16_t i = 0; i < samples; i++) {
    sx += analogRead(PIN_JOY_X);
    sy += analogRead(PIN_JOY_Y);
    delay(5);
  }
  centerX = (int)(sx / samples);
  centerY = (int)(sy / samples);

  filtX = centerX;
  filtY = centerY;
}

void joystick_init() {
  analogReadResolution(12);

  analogSetPinAttenuation(PIN_JOY_X, ADC_11db);
  analogSetPinAttenuation(PIN_JOY_Y, ADC_11db);

  pinMode(PIN_JOY_X, INPUT);
  pinMode(PIN_JOY_Y, INPUT);

  // IMPORTANT: keep joystick centered during boot
  joystick_calibrate(120);
}

motion_cmd_t joystick_read_command() {
  int x = read_adc_avg(PIN_JOY_X);
  int y = read_adc_avg(PIN_JOY_Y);

  // Low-pass filter
  filtX = (filtX * 7 + x) / 8;
  filtY = (filtY * 7 + y) / 8;

  // Your proven mapping from logs:
  // X axis changes for forward/back
  // Y axis changes for left/right
  int dx = filtX - centerX; // back(-) forward(+)
  int dy = filtY - centerY; // left(-) right(+)

  // Dead zone
  if (abs(dx) < DEAD && abs(dy) < DEAD) {
    return CMD_STOP;
  }

  // Forward/back active -> F/B + soft turns
  if (abs(dx) >= DEAD) {
    bool forward = (dx > 0);

    if (abs(dy) < TURN) {
      return forward ? CMD_F : CMD_B;
    }

    if (dy < 0) return forward ? CMD_LF : CMD_LB;
    return forward ? CMD_RF : CMD_RB;
  }

  // Only left/right -> hard spin
  if (dy < 0) return CMD_LH;
  return CMD_RH;
}
