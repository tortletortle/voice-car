#include "cmd_router.h"
#include "link_stm32.h"
#include "cmd_router_core.h"

static vc_router_state_t s_router;

void cmd_router_init(void)
{
  vc_router_state_init(&s_router);
}

void cmd_router_submit(vc_cmd_t cmd)
{
  (void)vc_router_state_submit(&s_router, cmd);
}

void cmd_router_poll(void)
{
  vc_cmd_t cmd;
  while (vc_router_state_take(&s_router, &cmd)) {
    int ok = link_stm32_send(cmd);
    if (cmd == CMD_STOP) {
      vc_router_state_complete_stop(&s_router, ok ? 1u : 0u);
    }
  }
}
