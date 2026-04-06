#include "can_bus.h"

bool can_bus_init(int txPin, int rxPin) {
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)txPin, (gpio_num_t)rxPin, TWAI_MODE_NORMAL);

  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    return false;
  }
  if (twai_start() != ESP_OK) {
    return false;
  }
  return true;
}

bool can_bus_send(uint32_t id, const uint8_t* data, uint8_t len, uint32_t timeout_ms) {
  twai_message_t m;
  m.identifier = id;
  m.extd = 0;
  m.rtr = 0;
  m.data_length_code = len;
  for (int i = 0; i < len; i++) m.data[i] = data[i];
  m.flags = TWAI_MSG_FLAG_NONE;

  return (twai_transmit(&m, pdMS_TO_TICKS(timeout_ms)) == ESP_OK);
}

bool can_bus_recv(twai_message_t* outMsg, uint32_t timeout_ms) {
  return (twai_receive(outMsg, pdMS_TO_TICKS(timeout_ms)) == ESP_OK);
}
