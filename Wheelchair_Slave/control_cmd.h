#pragma once
#include <Arduino.h>

// Same command set used in master
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

static inline const char* cmd_to_str(motion_cmd_t c) {
  switch (c) {
    case CMD_STOP: return "STOP";
    case CMD_F:    return "F";
    case CMD_B:    return "B";
    case CMD_LH:   return "LH";
    case CMD_RH:   return "RH";
    case CMD_LF:   return "LF";
    case CMD_RF:   return "RF";
    case CMD_LB:   return "LB";
    case CMD_RB:   return "RB";
    default:       return "?";
  }
}
