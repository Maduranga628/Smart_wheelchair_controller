#pragma once
#include <Arduino.h>
#include "control_cmd.h"

// CAN IDs
static const uint32_t CAN_ID_GESTURE = 0x101;
static const uint32_t CAN_ID_VOICE   = 0x102;

// Payload format
static const uint8_t  CAN_MAGIC = 0xA5;

// motion_cmd_t -> byte
static inline uint8_t cmd_to_u8(motion_cmd_t c) {
  switch (c) {
    case CMD_STOP: return 0;
    case CMD_F:    return 3;
    case CMD_B:    return 4;
    case CMD_LH:   return 1;
    case CMD_RH:   return 2;
    case CMD_LF:   return 8;
    case CMD_RF:   return 5;
    case CMD_LB:   return 6;
    case CMD_RB:   return 8;
    default:       return 0;
  }
}

// byte -> motion_cmd_t
static inline motion_cmd_t u8_to_cmd(uint8_t b) {
  switch (b) {
    case 0: return CMD_STOP;
    case 1: return CMD_F;
    case 2: return CMD_B;
    case 3: return CMD_LH;
    case 4: return CMD_RH;
    case 5: return CMD_LF;
    case 6: return CMD_RF;
    case 7: return CMD_LB;
    case 8: return CMD_RB;
    default: return CMD_STOP;
  }
}
