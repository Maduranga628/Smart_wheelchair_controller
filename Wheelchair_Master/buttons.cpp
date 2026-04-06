#include "buttons.h"
#include "pins_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static QueueHandle_t qBtns;

// Debounce time (adjust if needed)
static const TickType_t DEBOUNCE_TICKS = pdMS_TO_TICKS(180);

// Track last accepted interrupt time for each button
static volatile TickType_t lastTickUp    = 0;
static volatile TickType_t lastTickDown  = 0;
static volatile TickType_t lastTickEnter = 0;
static volatile TickType_t lastTickBack  = 0;

static inline bool debounce_ok(volatile TickType_t* lastTick) {
  TickType_t now = xTaskGetTickCountFromISR();
  if ((now - *lastTick) < DEBOUNCE_TICKS) {
    return false;
  }
  *lastTick = now;
  return true;
}

static void IRAM_ATTR isr_btn_up() {
  if (!debounce_ok(&lastTickUp)) return;
  button_t b = BTN_UP;
  xQueueSendFromISR(qBtns, &b, nullptr);
}

static void IRAM_ATTR isr_btn_down() {
  if (!debounce_ok(&lastTickDown)) return;
  button_t b = BTN_DOWN;
  xQueueSendFromISR(qBtns, &b, nullptr);
}

static void IRAM_ATTR isr_btn_enter() {
  if (!debounce_ok(&lastTickEnter)) return;
  button_t b = BTN_ENTER;
  xQueueSendFromISR(qBtns, &b, nullptr);
}

static void IRAM_ATTR isr_btn_back() {
  if (!debounce_ok(&lastTickBack)) return;
  button_t b = BTN_BACK;
  xQueueSendFromISR(qBtns, &b, nullptr);
}

void buttons_init() {
  // Small queue is enough for UI
  qBtns = xQueueCreate(16, sizeof(button_t));

  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_ENTER, INPUT_PULLUP);
  pinMode(PIN_BTN_BACK, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_BTN_UP),    isr_btn_up,    FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_DOWN),  isr_btn_down,  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_ENTER), isr_btn_enter, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_BACK),  isr_btn_back,  FALLING);
}

bool buttons_read_event(button_t* outBtn, TickType_t waitTicks) {
  return xQueueReceive(qBtns, outBtn, waitTicks) == pdTRUE;
}
