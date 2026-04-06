#include "can_rx.h"
#include "can_bus.h"
#include "can_protocol.h"
#include "pins_master.h"

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

static volatile uint32_t g_lastGestureMs = 0;
static volatile uint32_t g_lastVoiceMs   = 0;
static volatile motion_cmd_t g_gestureCmd = CMD_STOP;
static volatile motion_cmd_t g_voiceCmd   = CMD_STOP;

static motion_cmd_t parse_old_ascii(uint8_t b) {
  // Accept your test format: 'F','B','L','R','I'
  switch ((char)b) {
    case 'F': return CMD_F;
    case 'B': return CMD_B;
    case 'L': return CMD_LH;     // old test = hard left
    case 'R': return CMD_RH;     // old test = hard right
    case 'I': return CMD_STOP;
    case 'S': return CMD_STOP;
    default:  return CMD_STOP;
  }
}

static bool decode_cmd(const twai_message_t& m, motion_cmd_t& out) {
  if (m.data_length_code == 0) return false;

  // New format: [0xA5, cmd]
  if (m.data_length_code >= 2 && m.data[0] == CAN_MAGIC) {
    out = u8_to_cmd(m.data[1]);
    return true;
  }

  // Old format (your test): 1 byte ASCII command in data[0]
  out = parse_old_ascii(m.data[0]);
  return true;
}

void can_rx_init() {
  // your pins: CAN_TX=11, CAN_RX=10
  bool ok = can_bus_init(PIN_CAN_TX, PIN_CAN_RX);
  if (!ok) {
    // keep safe: no CAN
    Serial.println("CAN init FAILED");
  } else {
    Serial.println("CAN init OK (500kbps)");
  }

  portENTER_CRITICAL(&mux);
  g_lastGestureMs = 0;
  g_lastVoiceMs = 0;
  g_gestureCmd = CMD_STOP;
  g_voiceCmd = CMD_STOP;
  portEXIT_CRITICAL(&mux);
}

void can_rx_poll() {
  twai_message_t msg;

  // Drain all pending messages quickly (non-blocking)
  while (can_bus_recv(&msg, 0)) {
    motion_cmd_t cmd;
    if (!decode_cmd(msg, cmd)) continue;

    uint32_t now = millis();

    portENTER_CRITICAL(&mux);
    if (msg.identifier == CAN_ID_GESTURE) {
      g_gestureCmd = cmd;
      g_lastGestureMs = now;
    } else if (msg.identifier == CAN_ID_VOICE) {
      g_voiceCmd = cmd;
      g_lastVoiceMs = now;
    }
    portEXIT_CRITICAL(&mux);
  }
}

bool can_get_recent_gesture(motion_cmd_t* outCmd, uint32_t maxAgeMs) {
  uint32_t t;
  motion_cmd_t c;
  portENTER_CRITICAL(&mux);
  t = g_lastGestureMs;
  c = g_gestureCmd;
  portEXIT_CRITICAL(&mux);

  if (t == 0) return false;
  if ((millis() - t) > maxAgeMs) return false;
  *outCmd = c;
  return true;
}

bool can_get_recent_voice(motion_cmd_t* outCmd, uint32_t maxAgeMs) {
  uint32_t t;
  motion_cmd_t c;
  portENTER_CRITICAL(&mux);
  t = g_lastVoiceMs;
  c = g_voiceCmd;
  portEXIT_CRITICAL(&mux);

  if (t == 0) return false;
  if ((millis() - t) > maxAgeMs) return false;
  *outCmd = c;
  return true;
}
