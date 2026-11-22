#include "link_stm32.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* PCB：U2.4=ESP32_RX(网)、U2.5=ESP32_TX(网) → UART1 用 GPIO4/5 */
#define LINK_UART_NUM   UART_NUM_2
#define LINK_TXD_GPIO   5 /* ESP32 TX → STM32 RX */
#define LINK_RXD_GPIO   4 /* ESP32 RX ← STM32 TX */
#define LINK_BAUD       115200

static const char *TAG = "link_stm32";
static uint8_t s_seq;

static void link_rx_task(void *arg)
{
  uint8_t buf[64];
  char line[16];
  int ln = 0;
  (void)arg;

  for (;;) {
    int n = uart_read_bytes(LINK_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(500));
    if (n <= 0) {
      continue;
    }
    for (int i = 0; i < n; ++i) {
      char c = (char)buf[i];
      if (c == '\n') {
        line[ln] = '\0';
        if (ln > 0) {
          ESP_LOGI(TAG, "stm32: %s", line);
        }
        ln = 0;
      } else if (c != '\r' && ln < (int)sizeof(line) - 1) {
        line[ln++] = c;
      }
    }
  }
}

void link_stm32_init(void)
{
  const uart_config_t cfg = {
    .baud_rate = LINK_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_ERROR_CHECK(uart_driver_install(LINK_UART_NUM, 512, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(LINK_UART_NUM, &cfg));
  ESP_ERROR_CHECK(uart_set_pin(LINK_UART_NUM, LINK_TXD_GPIO, LINK_RXD_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  xTaskCreate(link_rx_task, "link_rx", 3072, NULL, 10, NULL);
  ESP_LOGI(TAG, "UART link listening GPIO4 for STM32 LINK");
}

void link_stm32_send(vc_cmd_t cmd)
{
  uint8_t frame[VC_FRAME_LEN];
  vc_pack((uint8_t)cmd, s_seq++, frame);
  uart_write_bytes(LINK_UART_NUM, (const char *)frame, VC_FRAME_LEN);
}
