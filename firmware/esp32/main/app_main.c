#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

#include "voice_uart.h"
#include "remote_net.h"
#include "cmd_router.h"
#include "link_stm32.h"
#include "cmd_protocol.h"

static const char *TAG = "app";

/**
 * PC 经 CH343 发文本，绕过 ASR，直接验收舵机：
 *   VC UP / VC DOWN / VC NOD / VC SHAKE / VC YCTR / VC PCEN
 *   VC HI / HAPPY / CUTE / ANGRY / SLEEP / LISTEN / CONF / SURP / SAD / WINK / NO
 *   VC 0x21  （按协议十六进制）
 * 未识别的行仍由 bridge 转给 STM32（bringup 的 S1/S2 可用）。
 */
static int pc_vc_line(const char *line)
{
  vc_cmd_t cmd = CMD_NOP;
  const char *p;

  if (!line || strncmp(line, "VC", 2) != 0) {
    return 0;
  }
  p = line + 2;
  while (*p == ' ' || *p == '\t') {
    ++p;
  }
  if (*p == '\0') {
    return 0;
  }

  if (strcmp(p, "UP") == 0 || strcmp(p, "up") == 0) {
    cmd = CMD_GIMBAL_UP;
  } else if (strcmp(p, "DOWN") == 0 || strcmp(p, "DN") == 0 || strcmp(p, "down") == 0) {
    cmd = CMD_GIMBAL_DOWN;
  } else if (strcmp(p, "NOD") == 0 || strcmp(p, "nod") == 0) {
    cmd = CMD_GIMBAL_PITCH_NOD;
  } else if (strcmp(p, "SHAKE") == 0 || strcmp(p, "shake") == 0) {
    cmd = CMD_GIMBAL_YAW_SHAKE;
  } else if (strcmp(p, "YCTR") == 0 || strcmp(p, "YAWSTOP") == 0) {
    cmd = CMD_GIMBAL_YAW_STOP;
  } else if (strcmp(p, "PCEN") == 0 || strcmp(p, "CENTER") == 0) {
    cmd = CMD_GIMBAL_PITCH_CENTER;
  } else if (strcmp(p, "LEFT") == 0) {
    cmd = CMD_GIMBAL_LEFT;
  } else if (strcmp(p, "RIGHT") == 0 || strcmp(p, "RGHT") == 0) {
    cmd = CMD_GIMBAL_RIGHT;
  } else if (strcmp(p, "HI") == 0 || strcmp(p, "HELLO") == 0) {
    cmd = CMD_FACE_HELLO;
  } else if (strcmp(p, "HAPPY") == 0) {
    cmd = CMD_FACE_HAPPY;
  } else if (strcmp(p, "CUTE") == 0) {
    cmd = CMD_FACE_CUTE;
  } else if (strcmp(p, "ANGRY") == 0) {
    cmd = CMD_FACE_ANGRY;
  } else if (strcmp(p, "SLEEP") == 0) {
    cmd = CMD_FACE_SLEEP;
  } else if (strcmp(p, "LISTEN") == 0) {
    cmd = CMD_FACE_LISTEN;
  } else if (strcmp(p, "CONF") == 0) {
    cmd = CMD_FACE_CONFUSED;
  } else if (strcmp(p, "SURP") == 0) {
    cmd = CMD_FACE_SURPRISE;
  } else if (strcmp(p, "SAD") == 0) {
    cmd = CMD_FACE_SAD;
  } else if (strcmp(p, "WINK") == 0) {
    cmd = CMD_FACE_WINK;
  } else if (strcmp(p, "NO") == 0 || strcmp(p, "REJECT") == 0) {
    cmd = CMD_FACE_REJECT;
  } else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
    unsigned long v = strtoul(p, NULL, 16);
    if (!vc_cmd_is_known((uint8_t)v) || (vc_cmd_t)v == CMD_NOP) {
      ESP_LOGW(TAG, "VC bad hex %s", p);
      return 1;
    }
    cmd = (vc_cmd_t)v;
  } else {
    ESP_LOGW(TAG, "VC unknown '%s' (UP/DOWN/NOD/SHAKE/HAPPY/...)", p);
    return 1;
  }

  ESP_LOGI(TAG, "PC inject cmd=0x%02X", (unsigned)cmd);
  cmd_router_submit(cmd);
  return 1;
}

void app_main(void)
{
  ESP_LOGI(TAG, "voice_car ESP32: voice+VC+SoftAP web -> STM32");

  link_stm32_init();
  cmd_router_init();
  link_stm32_set_pc_line_hook(pc_vc_line);
  voice_uart_init();
  remote_net_init();

  while (1) {
    cmd_router_poll();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
