#include "cmd_router.h"
#include "link_stm32.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static QueueHandle_t s_q;

void cmd_router_init(void)
{
  s_q = xQueueCreate(1, sizeof(vc_cmd_t)); /* length 1 → overwrite = latest wins */
}

void cmd_router_submit(vc_cmd_t cmd)
{
  if (!s_q) return;
  xQueueOverwrite(s_q, &cmd);
}

void cmd_router_poll(void)
{
  vc_cmd_t cmd;
  if (!s_q) return;
  while (xQueueReceive(s_q, &cmd, 0) == pdTRUE) {
    link_stm32_send(cmd);
  }
}
