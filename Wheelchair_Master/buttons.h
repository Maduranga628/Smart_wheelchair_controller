#pragma once
#include <Arduino.h>

typedef enum { BTN_UP, BTN_DOWN, BTN_ENTER, BTN_BACK } button_t;

void buttons_init();
bool buttons_read_event(button_t* outBtn, TickType_t waitTicks);
