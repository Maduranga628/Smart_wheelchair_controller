#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_BMI270_Arduino_Library.h>

#include "control_cmd.h"
#include "pins_slave.h"
#include "slave_can_tx.h"

// ---------------- IMU ----------------
static BMI270 imu;

// Your test code used Wire.begin(38,37). Keep the same here:
static const int PIN_IMU_SDA = 38;
static const int PIN_IMU_SCL = 37;
static const uint8_t BMI270_ADDR = 0x69;

// ---------------- Gesture thresholds (tune later) ----------------
// Neutral deadzone
static const float X_NEUTRAL = 0.20f;
static const float Y_NEUTRAL = 0.20f;

// Turn thresholds
static const float X_TURN = 0.35f;   // start turning
static const float X_HARD = 0.70f;   // hard left/right

// Forward/back thresholds
static const float Y_FWD  = 0.25f;
static const float Y_BACK = -0.25f;

// ---------------- Voice testing (temporary) ----------------
// Use Serial to simulate voice keywords:
// Type in Serial Monitor and send:
//   g=go(F)  s=stop  l=left(LH)  r=right(RH)  b=back(B)
#define VOICE_SERIAL_TEST 1
static motion_cmd_t g_voice_cmd = CMD_STOP;
static uint32_t g_voice_last_ms = 0;
static const uint32_t VOICE_HOLD_MS = 400; // keep voice command for 0.4s

// ---------------- Debug print rate ----------------
static uint32_t lastPrintMs = 0;

// ---------------- Function prototypes (IMPORTANT) ----------------
motion_cmd_t gesture_compute_cmd();
motion_cmd_t voice_compute_cmd();

// ---------------- Tasks ----------------
void gesture_task(void* pv) {
  (void)pv;
  while (true) {
    motion_cmd_t c = gesture_compute_cmd();
    slave_set_gesture_cmd(c);
    vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz
  }
}

void voice_task(void* pv) {
  (void)pv;
  while (true) {
    motion_cmd_t c = voice_compute_cmd();
    slave_set_voice_cmd(c);
    vTaskDelay(pdMS_TO_TICKS(30)); // ~33 Hz
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // I2C for BMI270
  Wire.begin(PIN_IMU_SDA, PIN_IMU_SCL);
  Wire.setClock(400000);

  Serial.println("Slave boot...");
  Serial.println("Init BMI270...");

  int8_t err = imu.beginI2C(BMI270_ADDR);
  if (err != BMI2_OK) {
    Serial.print("BMI270 init failed, err=");
    Serial.println(err);
    while (1) { delay(500); }
  }

  Serial.println("BMI270 OK");

  // CAN init + start sender task
  slave_can_tx_init();

  // Start tasks + CAN TX task
  xTaskCreatePinnedToCore(gesture_task, "gesture", 4096, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(voice_task,   "voice",   4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(slave_can_tx_task, "can_tx", 4096, nullptr, 3, nullptr, 1);

#if VOICE_SERIAL_TEST
  Serial.println("VOICE SERIAL TEST enabled:");
  Serial.println("Send: g(go) s(stop) l(left) r(right) b(back)");
#endif

  Serial.println("Slave started");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ---------------- Gesture compute (placeholder using accel thresholds) ----------------
motion_cmd_t gesture_compute_cmd() {
  imu.getSensorData();

  // In your test you used accelX/accelY directly
  float x = imu.data.accelX;
  float y = imu.data.accelY;

  // Decide forward/back first
  bool forward = (y > Y_FWD);
  bool back    = (y < Y_BACK);

  // Decide turning magnitude
  float ax = fabsf(x);
  float ay = fabsf(y);

  // Neutral -> STOP
  if (!forward && !back && ax < X_NEUTRAL && ay < Y_NEUTRAL) {
    return CMD_STOP;
  }

  // Forward cases
  if (forward) {
    // right tilt (x positive)
    if (x > X_HARD) return CMD_RF;     // strong forward-right
    if (x > X_TURN) return CMD_RF;     // forward-right
    // left tilt (x negative)
    if (x < -X_HARD) return CMD_LF;    // strong forward-left
    if (x < -X_TURN) return CMD_LF;    // forward-left
    return CMD_F;
  }

  // Backward cases
  if (back) {
    if (x > X_HARD) return CMD_RB;     // backward-right
    if (x > X_TURN) return CMD_RB;
    if (x < -X_HARD) return CMD_LB;    // backward-left
    if (x < -X_TURN) return CMD_LB;
    return CMD_B;
  }

  // No forward/back -> rotate in place
  if (x > X_TURN)  return CMD_RH;
  if (x < -X_TURN) return CMD_LH;

  return CMD_STOP;
}

// ---------------- Voice compute (temporary serial test stub) ----------------
motion_cmd_t voice_compute_cmd() {
#if VOICE_SERIAL_TEST
  while (Serial.available()) {
    char ch = (char)Serial.read();
    ch = (char)tolower((unsigned char)ch);

    motion_cmd_t cmd = CMD_STOP;
    if (ch == 'g') cmd = CMD_F;      // go
    else if (ch == 's') cmd = CMD_STOP;
    else if (ch == 'l') cmd = CMD_LH;
    else if (ch == 'r') cmd = CMD_RH;
    else if (ch == 'b') cmd = CMD_B;

    g_voice_cmd = cmd;
    g_voice_last_ms = millis();
  }

  // hold last voice command a short time
  if (g_voice_last_ms != 0 && (millis() - g_voice_last_ms) <= VOICE_HOLD_MS) {
    return g_voice_cmd;
  }
  return CMD_STOP;
#else
  // TODO: replace with your keyword model output:
  // go->CMD_F stop->CMD_STOP left->CMD_LH right->CMD_RH back->CMD_B
  return CMD_STOP;
#endif
}
