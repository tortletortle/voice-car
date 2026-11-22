#include "voice_uart.h"
#include "cmd_router.h"
#include "link_stm32.h"
#include "voice_word_map.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

/* PCB（原理图）：VOICE_RX=GPIO17、VOICE_TX=GPIO18；H1: 5V/GND/VOICE_RX/VOICE_TX */
#define VOICE_UART_NUM   UART_NUM_1
#define VOICE_TXD_GPIO   18 /* VOICE_TX：ESP TX → 模块 RX */
#define VOICE_RXD_GPIO   17 /* VOICE_RX：ESP RX ← 模块 TX */
/* 2026-11-22 已烧 CI1302_中文_单麦_V00681_UART1_115200_2M.bin，用户确认改 115200 */
#define VOICE_BAUD       115200

/* 亚博 5 字节：AA 55 <mode> <id> FB。唤醒 AA 55 03 00 FB 不 submit。 */
#define ASR_FRAME_H0     0xAAu
#define ASR_FRAME_H1     0x55u
#define ASR_FRAME_TAIL   0xFBu
#define ASR_MODE_CMD     0x00u
#define ASR_MODE_WAKE    0x03u
#define ASR_ID_WAKE      0x00u

/* 启英泰伦串口协议 V1，8 字节：A5 FA ID Cmd DataL DataH Sum FB。
 * Sum = 前 6 字节累加和低 8 位。Cmd 0x81 的 DataL 是命令词 id。 */
#define V1_H0            0xA5u
#define V1_H1            0xFAu
#define V1_END           0xFBu
#define V1_CMD_ASR       0x81u

static const char *TAG = "voice_uart";

typedef struct {
  uint8_t st; /* 0..4 expect AA 55 mode id FB */
  uint8_t mode;
  uint8_t id;
} asr_sync_t;

typedef struct {
  uint8_t st; /* 0..7 */
  uint8_t raw[8];
  uint8_t last[8];
  uint8_t have_last;
} v1_sync_t;

static asr_sync_t s_asr;
static v1_sync_t s_v1;

static void pc_line(const char *s)
{
  char line[96];
  ESP_LOGI(TAG, "%s", s);
  snprintf(line, sizeof(line), "voice: %s\r\n", s);
  link_stm32_pc_write(line);
}

static void v1_handle(void)
{
  char msg[80];
  uint8_t sum = 0;
  uint8_t i;
  uint8_t cmd = s_v1.raw[3];
  uint8_t id_lo = s_v1.raw[4];
  uint8_t id_hi = s_v1.raw[5];
  vc_cmd_t mapped;

  if (s_v1.have_last && memcmp(s_v1.last, s_v1.raw, 8) == 0) {
    return;
  }
  memcpy(s_v1.last, s_v1.raw, 8);
  s_v1.have_last = 1u;

  for (i = 0; i < 6; ++i) {
    sum = (uint8_t)(sum + s_v1.raw[i]);
  }
  if (s_v1.raw[7] != V1_END || sum != s_v1.raw[6]) {
    snprintf(msg, sizeof(msg),
             "v1 cmd=0x%02X id=0x%02X%02X sum=%02X exp=%02X end=%02X bad",
             cmd, id_hi, id_lo, s_v1.raw[6], sum, s_v1.raw[7]);
    pc_line(msg);
    return;
  }
  if (cmd == V1_CMD_ASR && id_hi == 0u && vc_voice_id_to_cmd(id_lo, &mapped)) {
    snprintf(msg, sizeof(msg), "v1 id=0x%02X -> cmd=0x%02X submit",
             id_lo, (unsigned)mapped);
    pc_line(msg);
    cmd_router_submit(mapped);
    return;
  }
  snprintf(msg, sizeof(msg), "v1 cmd=0x%02X id=0x%02X%02X ok unmapped",
           cmd, id_hi, id_lo);
  pc_line(msg);
}

static void v1_feed_byte(uint8_t b)
{
  if (s_v1.st == 0u) {
    if (b == V1_H0) {
      s_v1.raw[0] = b;
      s_v1.st = 1u;
    }
    return;
  }
  if (s_v1.st == 1u) {
    if (b == V1_H1) {
      s_v1.raw[1] = b;
      s_v1.st = 2u;
    } else if (b == V1_H0) {
      s_v1.raw[0] = b;
      s_v1.st = 1u;
    } else {
      s_v1.st = 0u;
    }
    return;
  }
  s_v1.raw[s_v1.st] = b;
  s_v1.st++;
  if (s_v1.st >= 8u) {
    s_v1.st = 0u;
    v1_handle();
  }
}

static void asr_feed_byte(uint8_t b)
{
  char msg[64];

  switch (s_asr.st) {
    case 0:
      s_asr.st = (b == ASR_FRAME_H0) ? 1u : 0u;
      break;
    case 1:
      s_asr.st = (b == ASR_FRAME_H1) ? 2u : ((b == ASR_FRAME_H0) ? 1u : 0u);
      break;
    case 2:
      s_asr.mode = b;
      s_asr.st = 3u;
      break;
    case 3:
      s_asr.id = b;
      s_asr.st = 4u;
      break;
    case 4:
      if (b == ASR_FRAME_TAIL) {
        vc_cmd_t cmd;
        if (s_asr.mode == ASR_MODE_WAKE && s_asr.id == ASR_ID_WAKE) {
          pc_line("asr wake AA 55 03 00 FB (ignore)");
        } else if (s_asr.mode == ASR_MODE_CMD &&
                   vc_voice_id_to_cmd(s_asr.id, &cmd)) {
          snprintf(msg, sizeof(msg), "asr id=0x%02X -> cmd=0x%02X submit",
                   s_asr.id, (unsigned)cmd);
          pc_line(msg);
          cmd_router_submit(cmd);
        } else {
          snprintf(msg, sizeof(msg), "asr mode=0x%02X id=0x%02X unmapped",
                   s_asr.mode, s_asr.id);
          pc_line(msg);
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
  char msg[80];

  while (left > 0) {
    int n = uart_read_bytes(VOICE_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(100));
    left -= 100;
    if (n <= 0) {
      continue;
    }
    total += (unsigned)n;
    {
      int i;
      int hex_n = n > 16 ? 16 : n;
      int pos = 0;
      pos = snprintf(msg, sizeof(msg), "rx %d hex", n);
      for (i = 0; i < hex_n && pos > 0 && pos < (int)sizeof(msg) - 4; ++i) {
        pos += snprintf(msg + pos, sizeof(msg) - (size_t)pos, " %02X", buf[i]);
      }
      pc_line(msg);
    }
    for (int i = 0; i < n; ++i) {
      asr_feed_byte(buf[i]);
      v1_feed_byte(buf[i]);
    }
  }
  return total;
}

static void voice_task(void *arg)
{
  unsigned n;
  (void)arg;

  pc_line("probe 115200 3s — speak to mic");
  n = voice_listen(3000);
  if (n == 0) {
    pc_line("NO BYTES on GPIO17 @115200 — check module TX->GPIO17 / 5V / GND");
  } else {
    char msg[48];
    snprintf(msg, sizeof(msg), "ok 115200 bytes=%u", n);
    pc_line(msg);
  }

  for (;;) {
    n = voice_listen(5000);
    if (n == 0) {
      pc_line("listening GPIO17 (speak wake+cmd)");
    }
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
  s_asr.mode = 0u;
  s_asr.id = 0u;
  memset(&s_v1, 0, sizeof(s_v1));
  ESP_ERROR_CHECK(uart_driver_install(VOICE_UART_NUM, 1024, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(VOICE_UART_NUM, &cfg));
  ESP_ERROR_CHECK(uart_set_pin(VOICE_UART_NUM, VOICE_TXD_GPIO, VOICE_RXD_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  xTaskCreate(voice_task, "voice_uart", 4096, NULL, 10, NULL);
  pc_line("CI1302 UART ready RX=GPIO17 TX=GPIO18");
}
