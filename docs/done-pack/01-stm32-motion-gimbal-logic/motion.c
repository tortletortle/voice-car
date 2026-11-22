#include "motion.h"
#include "../Bsp/board.h"
#include "../Bsp/motor.h"

static uint32_t s_remain_ms;
static uint8_t s_active;

void motion_init(void)
{
  s_remain_ms = 0;
  s_active = 0;
  motor_all_stop();
}

void motion_on_cmd(vc_cmd_t cmd)
{
  uint32_t ms = MOTION_DEFAULT_MS;
  motor_dir_t l = MOTOR_STOP;
  motor_dir_t r = MOTOR_STOP;

  switch (cmd) {
  case CMD_STOP:
    /* motor_all_stop 内会 motor_stby(0) */
    motor_all_stop();
    s_active = 0;
    s_remain_ms = 0;
    return;
  case CMD_FWD:
    l = MOTOR_FWD;
    r = MOTOR_FWD;
    break;
  case CMD_BACK:
    l = MOTOR_BACK;
    r = MOTOR_BACK;
    break;
  case CMD_LEFT:
    l = MOTOR_BACK;
    r = MOTOR_FWD;
    ms = MOTION_TURN_MS;
    break;
  case CMD_RIGHT:
    l = MOTOR_FWD;
    r = MOTOR_BACK;
    ms = MOTION_TURN_MS;
    break;
  default:
    return;
  }

  /* motor_set 内会按需 motor_stby(1)；PWM 默认 ≤30%（共 5V） */
  motor_set(l, r, MOTION_PWM_DEFAULT);
  s_remain_ms = ms;
  s_active = 1;
}

void motion_tick(uint32_t elapsed_ms)
{
  if (!s_active) return;
  if (elapsed_ms >= s_remain_ms) {
    motor_all_stop(); /* 含 STBY 拉低 */
    s_remain_ms = 0;
    s_active = 0;
  } else {
    s_remain_ms -= elapsed_ms;
  }
}
