#pragma once
#include <Arduino.h>
#include "control_cmd.h"

// Start BLE UART (Nordic UART Service style)
void ble_init(const char* deviceName);

// Connection state
bool ble_is_connected();

// Returns true if we received a valid command within maxAgeMs
bool ble_get_recent_command(motion_cmd_t* outCmd, uint32_t maxAgeMs);

// Optional: send debug/status back to phone (if it subscribed to notifications)
void ble_send_text(const char* s);
