#include "motion.h"
#include "../Bsp/board.h"
#include "../Bsp/motor.h"

static uint32_t s_remain_ms;
static uint8_t s_active;
static motion_state_t s_state;
static uint8_t s_pwm;
static vc_cmd_t s_last_cmd;

void motion_init(void)
{
  s_remain_ms = 0;
  s_active = 0;
  s_state = MOTION_STATE_IDLE;
  s_pwm = 0;
  s_last_cmd = CMD_NOP;
  motor_all_stop();
}

void motion_on_cmd(vc_cmd_t cmd)
{
  uint32_t ms = MOTION_DEFAULT_MS;
  motor_dir_t l = MOTOR_STOP;
  motor_dir_t r = MOTOR_STOP;

  s_last_cmd = cmd;

  switch (cmd) {
  case CMD_STOP:
    motor_all_stop();
    s_active = 0;
    s_remain_ms = 0;
    s_state = MOTION_STATE_IDLE;
    s_pwm = 0;
    return;
  case CMD_FWD:
    l = MOTOR_FWD;
    r = MOTOR_FWD;
    s_state = MOTION_STATE_FWD;
    break;
  case CMD_BACK:
    l = MOTOR_BACK;
    r = MOTOR_BACK;
    s_state = MOTION_STATE_BACK;
    break;
  case CMD_LEFT:
    l = MOTOR_BACK;
    r = MOTOR_FWD;
    ms = MOTION_TURN_MS;
    s_state = MOTION_STATE_LEFT;
    break;
  case CMD_RIGHT:
    l = MOTOR_FWD;
    r = MOTOR_BACK;
    ms = MOTION_TURN_MS;
    s_state = MOTION_STATE_RIGHT;
    break;
  default:
    return;
  }

  s_pwm = (uint8_t)MOTION_PWM_DEFAULT;
  motor_set(l, r, s_pwm);
  s_remain_ms = ms;
  s_active = 1;
}

void motion_tick(uint32_t elapsed_ms)
{
  if (!s_active) return;
  if (elapsed_ms >= s_remain_ms) {
    motor_all_stop();
    s_remain_ms = 0;
    s_active = 0;
    s_state = MOTION_STATE_IDLE;
    s_pwm = 0;
    /* Keep s_last_cmd for OLED retention after timed stop (V0.3). */
  } else {
    s_remain_ms -= elapsed_ms;
  }
}

motion_state_t motion_state(void)
{
  return s_state;
}

uint8_t motion_pwm_percent(void)
{
  return s_pwm;
}

vc_cmd_t motion_last_cmd(void)
{
  return s_last_cmd;
}
