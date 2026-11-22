#include <assert.h>
#include <stdio.h>
#include "cmd_router_core.h"

static void test_command_ids_and_classes(void)
{
  assert(CMD_FWD == 0x02);
  assert(CMD_BACK == 0x03);
  assert(CMD_LEFT == 0x04);
  assert(CMD_RIGHT == 0x05);
  assert(CMD_GIMBAL_NOD == 0x16);
  assert(CMD_GIMBAL_SHAKE == 0x17);
  assert(CMD_GIMBAL_NOD_TWICE == 0x18);
  assert(CMD_GIMBAL_SHAKE_TWICE == 0x19);
  assert(CMD_GIMBAL_NEUTRAL == 0x1A);
  assert(CMD_GIMBAL_AGREE == CMD_GIMBAL_NOD);
  assert(CMD_GIMBAL_DISAGREE == CMD_GIMBAL_SHAKE);
  assert(CMD_GIMBAL_GREETING == CMD_GIMBAL_NOD_TWICE);
  assert(vc_cmd_is_motion(CMD_FWD));
  assert(!vc_cmd_is_motion(CMD_GIMBAL_NOD));
  assert(vc_cmd_is_gimbal(CMD_GIMBAL_SHAKE));
  assert(vc_cmd_is_known(CMD_GIMBAL_NEUTRAL));
  assert(!vc_cmd_is_known(0x7Fu));
}

static void test_motion_and_gimbal_do_not_overwrite_each_other(void)
{
  vc_router_state_t state;
  vc_cmd_t out = CMD_NOP;
  vc_router_state_init(&state);

  assert(vc_router_state_submit(&state, CMD_FWD));
  assert(vc_router_state_submit(&state, CMD_GIMBAL_NOD));
  assert(vc_router_state_take(&state, &out) && out == CMD_FWD);
  assert(vc_router_state_take(&state, &out) && out == CMD_GIMBAL_NOD);
  assert(!vc_router_state_take(&state, &out));
}

static void test_latest_command_wins_per_channel(void)
{
  vc_router_state_t state;
  vc_cmd_t out = CMD_NOP;
  vc_router_state_init(&state);

  assert(vc_router_state_submit(&state, CMD_FWD));
  assert(vc_router_state_submit(&state, CMD_BACK));
  assert(vc_router_state_submit(&state, CMD_GIMBAL_NOD));
  assert(vc_router_state_submit(&state, CMD_GIMBAL_SHAKE));
  assert(vc_router_state_take(&state, &out) && out == CMD_BACK);
  assert(vc_router_state_take(&state, &out) && out == CMD_GIMBAL_SHAKE);
}

static void test_stop_priority_and_gimbal_retention(void)
{
  vc_router_state_t state;
  vc_cmd_t out = CMD_NOP;
  vc_router_state_init(&state);

  assert(vc_router_state_submit(&state, CMD_FWD));
  assert(vc_router_state_submit(&state, CMD_GIMBAL_NOD_TWICE));
  assert(vc_router_state_submit(&state, CMD_STOP));
  assert(!vc_router_state_submit(&state, CMD_BACK));
  assert(vc_router_state_take(&state, &out) && out == CMD_STOP);
  assert(!vc_router_state_submit(&state, CMD_LEFT));
  vc_router_state_complete_stop(&state, 1u);
  assert(vc_router_state_take(&state, &out) && out == CMD_GIMBAL_NOD_TWICE);
  assert(!vc_router_state_take(&state, &out));
}

static void test_stop_retry(void)
{
  vc_router_state_t state;
  vc_cmd_t out = CMD_NOP;
  vc_router_state_init(&state);

  assert(vc_router_state_submit(&state, CMD_STOP));
  assert(vc_router_state_take(&state, &out) && out == CMD_STOP);
  vc_router_state_complete_stop(&state, 0u);
  assert(vc_router_state_take(&state, &out) && out == CMD_STOP);
  vc_router_state_complete_stop(&state, 1u);
  assert(!vc_router_state_take(&state, &out));
}

int main(void)
{
  test_command_ids_and_classes();
  test_motion_and_gimbal_do_not_overwrite_each_other();
  test_latest_command_wins_per_channel();
  test_stop_priority_and_gimbal_retention();
  test_stop_retry();
  puts("voice command router tests: PASS");
  return 0;
}
