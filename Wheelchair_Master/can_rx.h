#pragma once
#include <Arduino.h>
#include "control_cmd.h"

void can_rx_init();
void can_rx_poll();  // call often (inside control loop or its own task)

bool can_get_recent_gesture(motion_cmd_t* outCmd, uint32_t maxAgeMs);
bool can_get_recent_voice(motion_cmd_t* outCmd, uint32_t maxAgeMs);
