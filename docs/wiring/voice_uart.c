#include "voice_uart.h"
#include "cmd_router.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* PCB U2.10=VOICE_RX、U2.11=VOICE_TX → DevKit 按 GPIO10/11（与 GPIO4/5 同口径） */
#define VOICE_UART_NUM   UART_NUM_1
#define VOICE_TXD_GPIO   11 /* ESP32 TX → 模块 RX1 */
#define VOICE_RXD_GPIO   10 /* ESP32 RX ← 模块 TX1 */
#define VOICE_BAUD       115200

static const char *TAG = "voice_uart";

static unsigned voice_listen(int ms)
{
  uint8_t buf[64];
  unsigned total = 0;
  int left = ms;

  while (left > 0) {
    int n = uart_read_bytes(VOICE_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(100));
    left -= 100;
    if (n <= 0) {
      continue;
    }
    total += (unsigned)n;
    ESP_LOGI(TAG, "voice rx %d total=%u", n, total);
    ESP_LOG_BUFFER_HEX(TAG, buf, n > 24 ? 24 : n);
  }
  return total;
}

static void voice_task(void *arg)
{
  unsigned n;
  (void)arg;

  n = voice_listen(3000);
  if (n == 0) {
    ESP_LOGI(TAG, "voice silent at 115200, try 9600");
    ESP_ERROR_CHECK(uart_set_baudrate(VOICE_UART_NUM, 9600));
    n = voice_listen(3000);
    ESP_LOGI(TAG, "voice 9600 bytes=%u", n);
    if (n == 0) {
      ESP_ERROR_CHECK(uart_set_baudrate(VOICE_UART_NUM, VOICE_BAUD));
      ESP_LOGI(TAG, "voice no bytes on GPIO10");
    }
  } else {
    ESP_LOGI(TAG, "voice 115200 bytes=%u", n);
  }

  for (;;) {
    (void)voice_listen(2000);
  }
}

void voice_uart_init(void)
{
  const uart_config_t cfg = {
    .baud_rate = VOICE_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_ERROR_CHECK(uart_driver_install(VOICE_UART_NUM, 1024, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(VOICE_UART_NUM, &cfg));
  ESP_ERROR_CHECK(uart_set_pin(VOICE_UART_NUM, VOICE_TXD_GPIO, VOICE_RXD_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  xTaskCreate(voice_task, "voice_uart", 3072, NULL, 10, NULL);
  ESP_LOGI(TAG, "CI1302 UART ready (word map later)");
}
