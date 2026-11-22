#include "link_stm32.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

/* PCB：U2.4=ESP32_RX(网)、U2.5=ESP32_TX(网) → UART2 用 GPIO4/5 */
#define LINK_UART_NUM   UART_NUM_2
#define LINK_TXD_GPIO   5 /* ESP32 TX → STM32 RX */
#define LINK_RXD_GPIO   4 /* ESP32 RX ← STM32 TX */
#define LINK_BAUD       115200
/* CH343 接在 UART0（GPIO43 TX），电脑口是这一路，不是 USB-JTAG 日志口 */
#define PC_UART_NUM     UART_NUM_0
#define PC_TXD_GPIO     43
#define PC_RXD_GPIO     44
#define PC_BAUD         115200

static const char *TAG = "link_stm32";
static uint8_t s_seq;
static link_pc_line_hook_t s_pc_line_hook;

static void pc_write(const char *s)
{
  if (!s) return;
  uart_write_bytes(PC_UART_NUM, s, strlen(s));
}

void link_stm32_pc_write(const char *s)
{
  pc_write(s);
}

static void link_rx_task(void *arg)
{
  uint8_t buf[64];
  char line[80];
  int ln = 0;
  (void)arg;

  for (;;) {
    int n = uart_read_bytes(LINK_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(50));
    if (n <= 0) {
      continue;
    }
    for (int i = 0; i < n; ++i) {
      char c = (char)buf[i];
      if (c == '\n') {
        line[ln] = '\0';
        if (ln > 0) {
          ESP_LOGI(TAG, "stm32: %s", line);
          pc_write("stm32: ");
          pc_write(line);
          pc_write("\r\n");
        }
        ln = 0;
      } else if (c != '\r' && ln < (int)sizeof(line) - 1) {
        line[ln++] = c;
      }
    }
  }
}

/* PC CH343 → UART0。丢掉本机回显（stm32:/esp32:），其余整行转给 STM32。 */
static void pc_rx_task(void *arg)
{
  char line[80];
  int ln = 0;
  (void)arg;

  for (;;) {
    uint8_t b;
    int n = uart_read_bytes(PC_UART_NUM, &b, 1, pdMS_TO_TICKS(50));
    if (n <= 0) {
      continue;
    }
    char c = (char)b;
    if (c == '\n' || c == '\r') {
      line[ln] = '\0';
      if (ln > 0 && strncmp(line, "stm32:", 6) != 0 && strncmp(line, "esp32:", 6) != 0) {
        int consumed = 0;
        if (s_pc_line_hook) {
          consumed = s_pc_line_hook(line);
        }
        if (!consumed) {
          /* bringup 文本（S1/S2…）直通；App 二进制帧走 cmd_router */
          uart_write_bytes(LINK_UART_NUM, line, ln);
          uart_write_bytes(LINK_UART_NUM, "\r\n", 2);
        }
      }
      ln = 0;
    } else if (ln < (int)sizeof(line) - 1) {
      line[ln++] = c;
    } else {
      ln = 0;
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
  ESP_ERROR_CHECK(uart_driver_install(LINK_UART_NUM, 512, 512, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(LINK_UART_NUM, &cfg));
  ESP_ERROR_CHECK(uart_set_pin(LINK_UART_NUM, LINK_TXD_GPIO, LINK_RXD_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  const uart_config_t pc = {
    .baud_rate = PC_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_ERROR_CHECK(uart_driver_install(PC_UART_NUM, 512, 256, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(PC_UART_NUM, &pc));
  ESP_ERROR_CHECK(uart_set_pin(PC_UART_NUM, PC_TXD_GPIO, PC_RXD_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  pc_write("esp32: bridge up COM<->STM32\r\n");
  xTaskCreate(link_rx_task, "link_rx", 3072, NULL, 10, NULL);
  xTaskCreate(pc_rx_task, "pc_rx", 3072, NULL, 10, NULL);
  ESP_LOGI(TAG, "UART bridge: PC UART0 <-> STM32 UART2");
}

int link_stm32_send(vc_cmd_t cmd)
{
  uint8_t asset;
  uint8_t frame[VC_FRAME_LEN];
  int n;

  /*
   * 当前板上的 bringup 只认文本 FACE <0..22>。
   * 表情命令改发这一行，不夹 6 字节帧，避免二进制粘进同一行。
   */
  if (vc_cmd_face_asset((uint8_t)cmd, &asset)) {
    char line[16];
    int len = snprintf(line, sizeof(line), "FACE %u\r\n", (unsigned)asset);
    if (len <= 0 || len >= (int)sizeof(line)) {
      return 0;
    }
    n = uart_write_bytes(LINK_UART_NUM, line, (size_t)len);
    return (n == len) ? 1 : 0;
  }

  vc_pack((uint8_t)cmd, s_seq++, frame);
  n = uart_write_bytes(LINK_UART_NUM, (const char *)frame, VC_FRAME_LEN);
  /* Local write-complete ≠ STM32 executed; still gates STOP retry. */
  return (n == (int)VC_FRAME_LEN) ? 1 : 0;
}

void link_stm32_set_pc_line_hook(link_pc_line_hook_t hook)
{
  s_pc_line_hook = hook;
}
