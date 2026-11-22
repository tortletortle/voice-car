#include "cmd_router_core.h"

void vc_router_state_init(vc_router_state_t *state)
{
  if (state == 0) return;
  state->normal_cmd = CMD_NOP;
  state->normal_pending = 0u;
  state->stop_pending = 0u;
  state->stop_in_flight = 0u;
}

uint8_t vc_router_state_submit(vc_router_state_t *state, vc_cmd_t cmd)
{
  if (state == 0 || !vc_cmd_is_known((uint8_t)cmd) || cmd == CMD_NOP) return 0u;

  if (cmd == CMD_STOP) {
    state->stop_pending = 1u;
    state->normal_pending = 0u;
    state->normal_cmd = CMD_NOP;
    return 1u;
  }

  /* Do not let any queued movement/gimbal command replace a pending STOP. */
  if (state->stop_pending || state->stop_in_flight) return 0u;
  state->normal_cmd = cmd; /* latest normal command wins */
  state->normal_pending = 1u;
  return 1u;
}

uint8_t vc_router_state_take(vc_router_state_t *state, vc_cmd_t *cmd_out)
{
  if (state == 0 || cmd_out == 0) return 0u;

  if (state->stop_pending) {
    *cmd_out = CMD_STOP;
    state->stop_pending = 0u;
    state->stop_in_flight = 1u;
    state->normal_pending = 0u;
    state->normal_cmd = CMD_NOP;
    return 1u;
  }
  if (state->normal_pending) {
    *cmd_out = state->normal_cmd;
    state->normal_pending = 0u;
    state->normal_cmd = CMD_NOP;
    return 1u;
  }
  return 0u;
}

void vc_router_state_complete_stop(vc_router_state_t *state, uint8_t success)
{
  if (state == 0 || !state->stop_in_flight) return;
  state->stop_in_flight = 0u;
  if (!success) state->stop_pending = 1u;
}
