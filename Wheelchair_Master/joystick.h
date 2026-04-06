#pragma once
#include <Arduino.h>
#include "control_cmd.h"

void joystick_init();
void joystick_calibrate(uint16_t samples = 80);
motion_cmd_t joystick_read_command();
