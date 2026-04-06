#pragma once
#include <Arduino.h>

void battery_init();
float battery_read_volts();      // filtered / averaged
uint8_t battery_percent_12v(float v);  // optional estimate
