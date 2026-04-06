#pragma once

// =====================
// OLED SSD1306 (I2C)
// =====================
static const int PIN_OLED_SDA = 7;
static const int PIN_OLED_SCL = 6;

// =====================
// Buttons (GPIO + INPUT_PULLUP)
// =====================
// BUTTON1 = UP
static const int PIN_BTN_UP    = 2;
// BUTTON2 = DOWN
static const int PIN_BTN_DOWN  = 4;
// BUTTON3 = ENTER
static const int PIN_BTN_ENTER = 12;
// BUTTON4 = BACK
static const int PIN_BTN_BACK  = 13;

// =====================
// Joystick (Analog)
// =====================
// Joystick switch pin (IO14) was marked "Do Not Use" in your pinout
static const int PIN_JOY_X = 15;
static const int PIN_JOY_Y = 16;

// =====================
// Battery sensing (ADC)
// =====================
static const int PIN_BAT_SENSE = 18;

// =====================
// CAN (TWAI)
// =====================
// Your sheet showed CAN RX = IO10 and CAN TX = IO11 on the bus side.
// Master needs both for TWAI.
static const int PIN_CAN_RX = 10;
static const int PIN_CAN_TX = 11;

// =====================
// Stepper Driver 1 (Motor 1)
// EN/DIR/PUL signals
// =====================
// LEFT motor = M1
static const int PIN_M1_EN  = 45;
static const int PIN_M1_DIR = 42;
static const int PIN_M1_PUL = 1;


// =====================
// Stepper Driver 2 (Motor 2)
// EN/DIR/PUL signals
// =====================

// RIGHT motor = M2
static const int PIN_M2_EN  = 48;
static const int PIN_M2_DIR = 38;
static const int PIN_M2_PUL = 39;
