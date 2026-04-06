#pragma once
#include <Arduino.h>
#include "control_cmd.h"

void motor_init_safe();
void motor_enable(bool enable);
bool motor_is_enabled();

void motor_apply_command(motion_cmd_t cmd);
