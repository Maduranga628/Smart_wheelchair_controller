#pragma once
#include <Arduino.h>
#include "control_cmd.h"

void slave_can_tx_init();
void slave_set_gesture_cmd(motion_cmd_t c);
void slave_set_voice_cmd(motion_cmd_t c);
void slave_can_tx_task(void* pv);
