#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "voice_uart.h"
#include "remote_net.h"
#include "cmd_router.h"
#include "link_stm32.h"

static const char *TAG = "app";

void app_main(void)
{
  ESP_LOGI(TAG, "voice_car ESP32: voice+remote -> STM32");

  link_stm32_init();
  cmd_router_init();
  voice_uart_init();
  remote_net_init();

  while (1) {
    cmd_router_poll();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
