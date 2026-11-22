#ifndef GIMBAL_H
#define GIMBAL_H

#include <stdint.h>
#include "../../common/cmd_protocol.h"

typedef enum {
  GIMBAL_YAW_STATE_CENTER = 0, /* 回中 / 停在中位 */
  GIMBAL_YAW_STATE_LEFT,
  GIMBAL_YAW_STATE_RIGHT,
  GIMBAL_YAW_STATE_SHAKE
} gimbal_yaw_state_t;

void gimbal_init(void);
void gimbal_on_cmd(vc_cmd_t cmd);
void gimbal_tick(uint32_t elapsed_ms);

int gimbal_pitch_deg(void);
gimbal_yaw_state_t gimbal_yaw_state(void);

#endif
