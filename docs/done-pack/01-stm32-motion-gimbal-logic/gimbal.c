#include "gimbal.h"
#include "../Bsp/board.h"
#include "../Bsp/servo.h"

static int s_pitch;
static int s_yaw;

static int clampi(int v, int lo, int hi)
{
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void gimbal_init(void)
{
  s_pitch = GIMBAL_PITCH_CENTER;
  s_yaw = GIMBAL_YAW_CENTER;
  servo_set_deg(GIMBAL_PITCH_CH, (uint8_t)s_pitch);
  servo_set_deg(GIMBAL_YAW_CH, (uint8_t)s_yaw);
}

void gimbal_on_cmd(vc_cmd_t cmd)
{
  switch (cmd) {
  case CMD_GIMBAL_UP:
    s_pitch = clampi(s_pitch + GIMBAL_STEP_DEG, GIMBAL_PITCH_MIN, GIMBAL_PITCH_MAX);
    break;
  case CMD_GIMBAL_DOWN:
    s_pitch = clampi(s_pitch - GIMBAL_STEP_DEG, GIMBAL_PITCH_MIN, GIMBAL_PITCH_MAX);
    break;
  case CMD_GIMBAL_LEFT:
    s_yaw = clampi(s_yaw + GIMBAL_STEP_DEG, GIMBAL_YAW_MIN, GIMBAL_YAW_MAX);
    break;
  case CMD_GIMBAL_RIGHT:
    s_yaw = clampi(s_yaw - GIMBAL_STEP_DEG, GIMBAL_YAW_MIN, GIMBAL_YAW_MAX);
    break;
  case CMD_GIMBAL_CENTER:
    s_pitch = GIMBAL_PITCH_CENTER;
    s_yaw = GIMBAL_YAW_CENTER;
    break;
  default:
    return;
  }
  servo_set_deg(GIMBAL_PITCH_CH, (uint8_t)s_pitch);
  servo_set_deg(GIMBAL_YAW_CH, (uint8_t)s_yaw);
}
