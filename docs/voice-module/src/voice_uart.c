#include "voice_uart.h"
#include "cmd_router.h"
#include "voice_word_map.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* PCB U2.10=VOICE_RX、U2.11=VOICE_TX → DevKit 按 GPIO10/11（与 GPIO4/5 同口径） */
#define VOICE_UART_NUM   UART_NUM_1
#define VOICE_TXD_GPIO   11 /* ESP32 TX → 模块 RX1 */
#define VOICE_RXD_GPIO   10 /* ESP32 RX ← 模块 TX1 */
#define VOICE_BAUD       115200

/* M5 Module ASR 风格：AA 55 ID 55 AA；唤醒等非车控 ID 不 submit */
#define ASR_FRAME_H0     0xAAu
#define ASR_FRAME_H1     0x55u
#define ASR_FRAME_T0     0x55u
#define ASR_FRAME_T1     0xAAu
#define ASR_ID_WAKE      0xFFu

static const char *TAG = "voice_uart";

typedef struct {
  uint8_t st; /* 0..4 expect H0 H1 ID T0 T1 */
  uint8_t id;
} asr_sync_t;

static asr_sync_t s_asr;

static void asr_feed_byte(uint8_t b)
{
  switch (s_asr.st) {
    case 0:
      s_asr.st = (b == ASR_FRAME_H0) ? 1u : 0u;
      break;
    case 1:
      s_asr.st = (b == ASR_FRAME_H1) ? 2u : ((b == ASR_FRAME_H0) ? 1u : 0u);
      break;
    case 2:
      s_asr.id = b;
      s_asr.st = 3u;
      break;
    case 3:
      s_asr.st = (b == ASR_FRAME_T0) ? 4u : ((b == ASR_FRAME_H0) ? 1u : 0u);
      break;
    case 4:
      if (b == ASR_FRAME_T1) {
        vc_cmd_t cmd;
        if (s_asr.id == ASR_ID_WAKE) {
          ESP_LOGI(TAG, "asr wake id=0x%02X (ignore)", s_asr.id);
        } else if (vc_voice_id_to_cmd(s_asr.id, &cmd)) {
          ESP_LOGI(TAG, "asr id=0x%02X -> cmd=0x%02X submit", s_asr.id, (unsigned)cmd);
          cmd_router_submit(cmd);
        } else {
          ESP_LOGW(TAG, "asr id=0x%02X unmapped", s_asr.id);
        }
        s_asr.st = 0u;
      } else {
        s_asr.st = (b == ASR_FRAME_H0) ? 1u : 0u;
      }
      break;
    default:
      s_asr.st = 0u;
      break;
  }
}

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
    for (int i = 0; i < n; ++i) {
      asr_feed_byte(buf[i]);
    }
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
  s_asr.st = 0u;
  s_asr.id = 0u;
  ESP_ERROR_CHECK(uart_driver_install(VOICE_UART_NUM, 1024, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(VOICE_UART_NUM, &cfg));
  ESP_ERROR_CHECK(uart_set_pin(VOICE_UART_NUM, VOICE_TXD_GPIO, VOICE_RXD_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  xTaskCreate(voice_task, "voice_uart", 3072, NULL, 10, NULL);
  ESP_LOGI(TAG, "CI1302 UART ready (M5 AA55 frame + simple map)");
}
