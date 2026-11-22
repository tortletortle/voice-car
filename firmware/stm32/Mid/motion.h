#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>
#include "../../common/cmd_protocol.h"

typedef enum {
  MOTION_STATE_IDLE = 0,
  MOTION_STATE_FWD,
  MOTION_STATE_BACK,
  MOTION_STATE_LEFT,
  MOTION_STATE_RIGHT
} motion_state_t;

void motion_init(void);
void motion_on_cmd(vc_cmd_t cmd);
void motion_tick(uint32_t elapsed_ms);

motion_state_t motion_state(void);
uint8_t motion_pwm_percent(void);
vc_cmd_t motion_last_cmd(void);

#endif
