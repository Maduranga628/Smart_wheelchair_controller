#include "slave_can_tx.h"
#include "can_bus.h"
#include "can_protocol.h"
#include "pins_slave.h"

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static volatile motion_cmd_t g_gesture = CMD_STOP;
static volatile motion_cmd_t g_voice   = CMD_STOP;

void slave_can_tx_init() {
  bool ok = can_bus_init(PIN_CAN_TX, PIN_CAN_RX);
  if (ok) Serial.println("SLAVE CAN OK (500kbps)");
  else    Serial.println("SLAVE CAN FAIL");
}

void slave_set_gesture_cmd(motion_cmd_t c) {
  portENTER_CRITICAL(&mux);
  g_gesture = c;
  portEXIT_CRITICAL(&mux);
}

void slave_set_voice_cmd(motion_cmd_t c) {
  portENTER_CRITICAL(&mux);
  g_voice = c;
  portEXIT_CRITICAL(&mux);
}

static void send_cmd(uint32_t id, motion_cmd_t c) {
  uint8_t data[2];
  data[0] = CAN_MAGIC;
  data[1] = cmd_to_u8(c);
  can_bus_send(id, data, 2, 5);
}

void slave_can_tx_task(void* pv) {
  (void)pv;
  const TickType_t period = pdMS_TO_TICKS(50); // 20Hz

  while (true) {
    motion_cmd_t g, v;
    portENTER_CRITICAL(&mux);
    g = g_gesture;
    v = g_voice;
    portEXIT_CRITICAL(&mux);

    send_cmd(CAN_ID_GESTURE, g);
    send_cmd(CAN_ID_VOICE, v);

    vTaskDelay(period);
  }
}
