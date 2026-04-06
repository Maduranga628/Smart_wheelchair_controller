#include "control_loop.h"

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ui_menu.h"
#include "motor_stepper.h"
#include "joystick.h"
#include "ble_uart.h"
#include "can_rx.h"
#include "control_cmd.h"

// ---------------- TUNING ----------------

// BLE is highest priority when it is actively sending commands.
static const uint32_t BLE_ACTIVE_TIMEOUT_MS = 350;

// If selected mode is BLE, same deadman timeout.
static const uint32_t BLE_MODE_TIMEOUT_MS  = 350;

// CAN deadman timeouts (slave must keep sending frames)
static const uint32_t CAN_GESTURE_TIMEOUT_MS = 300;
static const uint32_t CAN_VOICE_TIMEOUT_MS   = 300;

// Loop rate
static const uint32_t CONTROL_PERIOD_MS = 50;

// Debug logs (0 = off, 1 = on)
#define CONTROL_DEBUG 0

// ---------------- INTERNAL ----------------
static motion_cmd_t g_lastApplied = CMD_STOP;

static void apply_cmd_if_changed(motion_cmd_t c) {
  if (c == g_lastApplied) return;
  g_lastApplied = c;
  motor_apply_command(c);

#if CONTROL_DEBUG
  Serial.print("APPLY ");
  Serial.println(cmd_to_str(c));
#endif
}

static bool ble_get_active_cmd(motion_cmd_t* outCmd) {
  motion_cmd_t c = CMD_STOP;
  if (ble_is_connected() && ble_get_recent_command(&c, BLE_ACTIVE_TIMEOUT_MS)) {
    *outCmd = c;
    return true;
  }
  return false;
}

static motion_cmd_t read_selected_mode_cmd(control_mode_t mode) {
  // Always poll CAN to keep RX queue clean
  can_rx_poll();

  switch (mode) {
    case MODE_JOYSTICK:
      // joystick module should return STOP when centered
      return joystick_read_command();

    case MODE_BLE: {
      motion_cmd_t c = CMD_STOP;
      if (ble_is_connected() && ble_get_recent_command(&c, BLE_MODE_TIMEOUT_MS)) return c;
      return CMD_STOP;
    }

    case MODE_GESTURE: {
      motion_cmd_t c = CMD_STOP;
      if (can_get_recent_gesture(&c, CAN_GESTURE_TIMEOUT_MS)) return c;
      return CMD_STOP;
    }

    case MODE_VOICE: {
      motion_cmd_t c = CMD_STOP;
      if (can_get_recent_voice(&c, CAN_VOICE_TIMEOUT_MS)) return c;
      return CMD_STOP;
    }

    default:
      return CMD_STOP;
  }
}

static void control_task(void* pv) {
  (void)pv;

  uint32_t lastAlive = 0;

  while (true) {
    // Safety first
    if (!ui_is_armed()) {
      apply_cmd_if_changed(CMD_STOP);
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    // 1) BLE caregiver override (highest priority)
    motion_cmd_t bleCmd = CMD_STOP;
    if (ble_get_active_cmd(&bleCmd)) {
      apply_cmd_if_changed(bleCmd);
    } else {
      // 2) Otherwise follow selected mode
      control_mode_t mode = ui_get_selected_mode();
      motion_cmd_t cmd = read_selected_mode_cmd(mode);
      apply_cmd_if_changed(cmd);
    }

#if CONTROL_DEBUG
    uint32_t now = millis();
    if (now - lastAlive >= 1000) {
      lastAlive = now;
      Serial.print("ALIVE mode=");
      Serial.print((int)ui_get_selected_mode());
      Serial.print(" armed=");
      Serial.print(ui_is_armed() ? 1 : 0);
      Serial.print(" lastCmd=");
      Serial.println(cmd_to_str(g_lastApplied));
    }
#endif

    vTaskDelay(pdMS_TO_TICKS(CONTROL_PERIOD_MS));
  }
}

void control_loop_start() {
  // Init CAN RX on master (TX=11 RX=10 from pins_master.h)
  can_rx_init();

  // Control task on core 1 (UI is on core 0 in your setup)
  xTaskCreatePinnedToCore(
    control_task,
    "ControlTask",
    4096,
    nullptr,
    3,
    nullptr,
    1
  );
}
