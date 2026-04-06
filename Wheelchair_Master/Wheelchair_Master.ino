#include "motor_stepper.h"
#include "buttons.h"
#include "ui_menu.h"
#include "joystick.h"
#include "control_loop.h"
#include "ble_uart.h"


void setup() {
  Serial.begin(115200);

  motor_init_safe();
  buttons_init();
  ui_init();
  joystick_init();

  ble_init("Wheelchair");

  xTaskCreatePinnedToCore(ui_task, "UITask", 4096, nullptr, 2, nullptr, 0);

  control_loop_start();

  Serial.println("Master started");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
