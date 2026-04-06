#pragma once
#include <Arduino.h>
#include "driver/twai.h"

bool can_bus_init(int txPin, int rxPin);
bool can_bus_send(uint32_t id, const uint8_t* data, uint8_t len, uint32_t timeout_ms);
bool can_bus_recv(twai_message_t* outMsg, uint32_t timeout_ms);
