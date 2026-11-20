#ifndef VOICE_CAR_CMD_ROUTER_CORE_H
#define VOICE_CAR_CMD_ROUTER_CORE_H

#include <stdint.h>
#include "cmd_protocol.h"

typedef struct {
  vc_cmd_t normal_cmd;
  uint8_t normal_pending;
  uint8_t stop_pending;
  uint8_t stop_in_flight;
} vc_router_state_t;

void vc_router_state_init(vc_router_state_t *state);
/* Returns 1 if accepted, 0 for invalid/NOP or a normal command blocked by STOP. */
uint8_t vc_router_state_submit(vc_router_state_t *state, vc_cmd_t cmd);
/* STOP is returned before normal commands and clears any stale normal command. */
uint8_t vc_router_state_take(vc_router_state_t *state, vc_cmd_t *cmd_out);
/* Complete the transmitted STOP; failed transmission is re-queued for retry. */
void vc_router_state_complete_stop(vc_router_state_t *state, uint8_t success);

#endif
