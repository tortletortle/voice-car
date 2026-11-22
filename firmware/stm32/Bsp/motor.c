#include "motor.h"
#include "board.h"

static uint8_t s_stby;

void motor_init(void)
{
  s_stby = 0;
  motor_stby(0);
}

void motor_stby(int enable)
{
  if (!MOTOR_GPIO_CONFIGURED) {
    s_stby = 0;
    return;
  }
  s_stby = enable ? 1u : 0u;
  (void)MOTOR_STBY_ACTIVE_HIGH;
  (void)s_stby;
  /* TODO: drive STBY GPIO after PCB pin map is filled. */
}

void motor_all_stop(void)
{
  motor_set(MOTOR_STOP, MOTOR_STOP, 0);
}

void motor_set(motor_dir_t left, motor_dir_t right, uint8_t pwm_percent)
{
  motor_dir_t l = left;
  motor_dir_t r = right;

  /* Cloud V0.3 safety + local MotA/MotB policy: no GPIO map → fail-closed. */
  if (!MOTOR_GPIO_CONFIGURED) {
    (void)l;
    (void)r;
    (void)pwm_percent;
    s_stby = 0;
    return;
  }

  if (MOTOR_SWAP_LR) {
    motor_dir_t t = l;
    l = r;
    r = t;
  }
  if (MOTOR_INVERT_L) {
    if (l == MOTOR_FWD) l = MOTOR_BACK;
    else if (l == MOTOR_BACK) l = MOTOR_FWD;
  }
  if (MOTOR_INVERT_R) {
    if (r == MOTOR_FWD) r = MOTOR_BACK;
    else if (r == MOTOR_BACK) r = MOTOR_FWD;
  }

  if (!MOTOR_USE_CH_A || !MOTOR_USE_CH_B) {
    return;
  }
  (void)MOTOR_USE_CH_C;
  (void)MOTOR_USE_CH_D;

  if (l == MOTOR_STOP && r == MOTOR_STOP) {
    pwm_percent = 0;
    motor_stby(0);
    return;
  }

  motor_stby(1);

  if (pwm_percent > MOTION_PWM_DEFAULT) {
    pwm_percent = MOTION_PWM_DEFAULT;
  }

  (void)l;
  (void)r;
  (void)pwm_percent;
  /* TODO: MotA (AIN1/2+PWA) / MotB (BIN1/2+PWB) from PCB netlist. */
}
