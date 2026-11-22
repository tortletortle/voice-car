#include "gimbal.h"
#include "../Bsp/board.h"
#include "../Bsp/servo.h"

static int s_pitch;
static gimbal_yaw_state_t s_yaw;
static uint16_t s_yaw_us;
static uint8_t s_shake_phase; /* 0=idle; 1=L 2=R 3=L 4=center */
static uint32_t s_shake_hold_ms;
static uint8_t s_nod_phase; /* 0=idle; 1=低头 2=抬头 3=回中 */
static uint32_t s_nod_hold_ms;

static int clampi(int v, int lo, int hi)
{
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void apply_pitch(int deg)
{
  s_pitch = clampi(deg, GIMBAL_PITCH_MIN, GIMBAL_PITCH_MAX);
  servo_set_deg(GIMBAL_PITCH_CH, s_pitch);
}

static void apply_yaw_us(uint16_t us, gimbal_yaw_state_t st)
{
  s_yaw_us = us;
  s_yaw = st;
  servo_set_pulse_us(GIMBAL_YAW_CH, us);
}

static void nod_cancel(void)
{
  s_nod_phase = 0u;
  s_nod_hold_ms = 0u;
}

static void shake_cancel(void)
{
  s_shake_phase = 0u;
  s_shake_hold_ms = 0u;
}

static void yaw_hold(uint16_t us, gimbal_yaw_state_t st)
{
  shake_cancel();
  apply_yaw_us(us, st);
}

static void shake_enter_phase(uint8_t phase)
{
  s_shake_phase = phase;
  s_shake_hold_ms = GIMBAL_YAW_SHAKE_HOLD_MS;
  s_yaw = GIMBAL_YAW_STATE_SHAKE;
  if (phase == 1u || phase == 3u) {
    apply_yaw_us((uint16_t)GIMBAL_YAW_LEFT_US, GIMBAL_YAW_STATE_SHAKE);
  } else if (phase == 2u) {
    apply_yaw_us((uint16_t)GIMBAL_YAW_RIGHT_US, GIMBAL_YAW_STATE_SHAKE);
  } else {
    apply_yaw_us((uint16_t)GIMBAL_YAW_CENTER_US, GIMBAL_YAW_STATE_CENTER);
    shake_cancel();
  }
}

static void yaw_start_shake(void)
{
  nod_cancel(); /* 避免俯仰/偏航动画同时抢舵机节奏 */
  shake_enter_phase(1u);
}

/* 点头：低头(0°) → 抬头(-45°) → 回中；脉宽只用已手调 500/750/1000µs */
static void nod_enter_phase(uint8_t phase)
{
  s_nod_phase = phase;
  s_nod_hold_ms = GIMBAL_PITCH_NOD_HOLD_MS;
  if (phase == 1u) {
    apply_pitch(GIMBAL_PITCH_DOWN);
  } else if (phase == 2u) {
    apply_pitch(GIMBAL_PITCH_UP);
  } else {
    apply_pitch(GIMBAL_PITCH_CENTER);
    nod_cancel();
  }
}

static void pitch_start_nod(void)
{
  shake_cancel();
  nod_enter_phase(1u);
}

void gimbal_init(void)
{
  s_pitch = GIMBAL_PITCH_CENTER;
  s_yaw = GIMBAL_YAW_STATE_CENTER;
  s_yaw_us = (uint16_t)GIMBAL_YAW_CENTER_US;
  shake_cancel();
  nod_cancel();
  apply_pitch(s_pitch);
  apply_yaw_us(s_yaw_us, GIMBAL_YAW_STATE_CENTER);
}

void gimbal_on_cmd(vc_cmd_t cmd)
{
  switch (cmd) {
  case CMD_GIMBAL_UP:
    /* 抬头：一次到 -45°（语音「抬头」要肉眼可见；脉宽 500µs 已手调） */
    nod_cancel();
    apply_pitch(GIMBAL_PITCH_UP);
    break;
  case CMD_GIMBAL_DOWN:
    /* 低头：一次到 0°（脉宽 1000µs 已手调） */
    nod_cancel();
    apply_pitch(GIMBAL_PITCH_DOWN);
    break;
  case CMD_GIMBAL_LEFT:
    yaw_hold((uint16_t)GIMBAL_YAW_LEFT_US, GIMBAL_YAW_STATE_LEFT);
    break;
  case CMD_GIMBAL_RIGHT:
    yaw_hold((uint16_t)GIMBAL_YAW_RIGHT_US, GIMBAL_YAW_STATE_RIGHT);
    break;
  case CMD_GIMBAL_YAW_STOP:
    yaw_hold((uint16_t)GIMBAL_YAW_CENTER_US, GIMBAL_YAW_STATE_CENTER);
    break;
  case CMD_GIMBAL_YAW_SHAKE:
    yaw_start_shake();
    break;
  case CMD_GIMBAL_PITCH_NOD:
    pitch_start_nod();
    break;
  case CMD_GIMBAL_PITCH_CENTER:
    /* V0.3: CENTER = pitch-center only */
    nod_cancel();
    apply_pitch(GIMBAL_PITCH_CENTER);
    break;
  default:
    break;
  }
}

void gimbal_tick(uint32_t elapsed_ms)
{
  if (s_shake_phase != 0u) {
    if (elapsed_ms >= s_shake_hold_ms) {
      if (s_shake_phase >= 3u) {
        shake_enter_phase(4u); /* 回中并结束 */
      } else {
        shake_enter_phase((uint8_t)(s_shake_phase + 1u));
      }
    } else {
      s_shake_hold_ms -= elapsed_ms;
    }
  }

  if (s_nod_phase != 0u) {
    if (elapsed_ms >= s_nod_hold_ms) {
      if (s_nod_phase >= 2u) {
        nod_enter_phase(3u); /* 回中并结束 */
      } else {
        nod_enter_phase((uint8_t)(s_nod_phase + 1u));
      }
    } else {
      s_nod_hold_ms -= elapsed_ms;
    }
  }
}

int gimbal_pitch_deg(void)
{
  return s_pitch;
}

gimbal_yaw_state_t gimbal_yaw_state(void)
{
  return s_yaw;
}
