#pragma once

typedef enum {
  MODE_NONE = 0,
  MODE_BLE,
  MODE_JOYSTICK,
  MODE_GESTURE,
  MODE_VOICE
} control_mode_t;

typedef enum {
  CMD_STOP = 0,
  CMD_F,
  CMD_B,
  CMD_LH,
  CMD_RH,
  CMD_LF,
  CMD_RF,
  CMD_LB,
  CMD_RB
} motion_cmd_t;

static inline const char* mode_to_str(control_mode_t m) {
  switch (m) {
    case MODE_BLE:      return "BLE";
    case MODE_JOYSTICK: return "JOYSTICK";
    case MODE_GESTURE:  return "GESTURE";
    case MODE_VOICE:    return "VOICE";
    default:            return "NONE";
  }
}

static inline const char* cmd_to_str(motion_cmd_t c) {
  switch (c) {
    case CMD_F:   return "F";
    case CMD_B:   return "B";
    case CMD_LH:  return "LH";
    case CMD_RH:  return "RH";
    case CMD_LF:  return "LF";
    case CMD_RF:  return "RF";
    case CMD_LB:  return "LB";
    case CMD_RB:  return "RB";
    default:      return "STOP";
  }
}
