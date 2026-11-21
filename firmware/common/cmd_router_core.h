#ifndef VOICE_CAR_CMD_ROUTER_CORE_H
#define VOICE_CAR_CMD_ROUTER_CORE_H

#include <stdint.h>
#include "cmd_protocol.h"

typedef struct {
  /* Keep the original fields first for source/layout compatibility. */
  vc_cmd_t normal_cmd;       /* latest pending chassis motion command */
  uint8_t normal_pending;
  uint8_t stop_pending;
  uint8_t stop_in_flight;
  /* Gimbal actions have an independent slot and cannot overwrite motion. */
  vc_cmd_t gimbal_cmd;
  uint8_t gimbal_pending;
} vc_router_state_t;

void vc_router_state_init(vc_router_state_t *state);
/* Returns 1 if accepted; STOP blocks new chassis motion until transmitted. */
uint8_t vc_router_state_submit(vc_router_state_t *state, vc_cmd_t cmd);
/* One frame at a time: STOP, then chassis motion, then gimbal action. */
uint8_t vc_router_state_take(vc_router_state_t *state, vc_cmd_t *cmd_out);
/* Complete transmitted STOP; failed transmission is re-queued for retry. */
void vc_router_state_complete_stop(vc_router_state_t *state, uint8_t success);

#endif
