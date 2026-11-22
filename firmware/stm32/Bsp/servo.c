#include "servo.h"
#include "board.h"

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_APB1ENR   REG32(0x4002101Cu)
#define RCC_APB2ENR   REG32(0x40021018u)
#define GPIOA_CRL     REG32(0x40010800u)
#define GPIOA_BSRR    REG32(0x40010810u)
#define TIM2_CR1      REG32(0x40000000u)
#define TIM2_EGR      REG32(0x40000014u)
#define TIM2_CCMR1    REG32(0x40000018u)
#define TIM2_CCER     REG32(0x40000020u)
#define TIM2_PSC      REG32(0x40000028u)
#define TIM2_ARR      REG32(0x4000002Cu)
#define TIM2_CCR1     REG32(0x40000034u)
#define TIM2_CCR2     REG32(0x40000038u)

#define RCC_IOPA      (1u << 2)
#define RCC_TIM2      (1u << 0)

/* bringup 同款：HSI 8 MHz /8 → 1 MHz 计数，ARR=19999 → 20 ms */
#define SERVO_PSC     7u
#define SERVO_ARR     19999u

static uint8_t s_ch1_on;
static uint8_t s_ch2_on;

static void tim2_base_on(void)
{
  TIM2_PSC = SERVO_PSC;
  TIM2_ARR = SERVO_ARR;
  TIM2_EGR = 1u;
  TIM2_CR1 = (1u << 7) | (1u << 0);
}

static void tim2_maybe_off(void)
{
  if (!s_ch1_on && !s_ch2_on) {
    TIM2_CCER = 0u;
    TIM2_CCR1 = 0u;
    TIM2_CCR2 = 0u;
    TIM2_CR1 = 0u;
  }
}

static uint16_t clamp_us(uint16_t us)
{
  if (us > SERVO_ARR) return (uint16_t)SERVO_ARR;
  return us;
}

static uint16_t pitch_deg_to_us(int deg)
{
  /* 线性：0°→1000µs，-45°→500µs */
  int us;
  if (deg > GIMBAL_PITCH_DOWN) deg = GIMBAL_PITCH_DOWN;
  if (deg < GIMBAL_PITCH_UP) deg = GIMBAL_PITCH_UP;
  /* us = 1000 + deg * (500-1000)/(-45-0) = 1000 + deg * (-500)/(-45) */
  us = (int)GIMBAL_PITCH_DOWN_US +
       (deg - GIMBAL_PITCH_DOWN) * ((int)GIMBAL_PITCH_UP_US - (int)GIMBAL_PITCH_DOWN_US) /
           (GIMBAL_PITCH_UP - GIMBAL_PITCH_DOWN);
  if (us < 0) us = 0;
  if (us > (int)SERVO_ARR) us = (int)SERVO_ARR;
  return (uint16_t)us;
}

static void pulse_ch1(uint16_t us)
{
  uint32_t cr;
  uint32_t ccmr;
  uint32_t ccer;

  us = clamp_us(us);
  cr = GPIOA_CRL;
  cr &= ~(0xFu << 0);
  cr |= (0xBu << 0); /* PA0 AF PP 50MHz */
  GPIOA_CRL = cr;
  tim2_base_on();
  TIM2_CCR1 = us;
  ccmr = TIM2_CCMR1;
  ccmr &= ~0xFFu;
  ccmr |= (0x6u << 4) | (1u << 3);
  TIM2_CCMR1 = ccmr;
  ccer = TIM2_CCER;
  ccer |= (1u << 0);
  TIM2_CCER = ccer;
  s_ch1_on = 1u;
}

static void pulse_ch2(uint16_t us)
{
  uint32_t cr;
  uint32_t ccmr;
  uint32_t ccer;

  us = clamp_us(us);
  cr = GPIOA_CRL;
  cr &= ~(0xFu << 4);
  cr |= (0xBu << 4); /* PA1 AF PP 50MHz */
  GPIOA_CRL = cr;
  tim2_base_on();
  TIM2_CCR2 = us;
  ccmr = TIM2_CCMR1;
  ccmr &= ~0xFF00u;
  ccmr |= (0x6u << 12) | (1u << 11);
  TIM2_CCMR1 = ccmr;
  ccer = TIM2_CCER;
  ccer |= (1u << 4);
  TIM2_CCER = ccer;
  s_ch2_on = 1u;
}

void servo_init(void)
{
  s_ch1_on = 0u;
  s_ch2_on = 0u;
  if (!VC_SERVO_OUTPUT_ENABLE) return;

  RCC_APB2ENR |= RCC_IOPA;
  RCC_APB1ENR |= RCC_TIM2;

  /* 初始拉低，避免上电乱跳 */
  {
    uint32_t cr = GPIOA_CRL;
    cr &= ~((0xFu << 0) | (0xFu << 4));
    cr |= (0x1u << 0) | (0x1u << 4); /* 输出 PP 10MHz */
    GPIOA_CRL = cr;
    GPIOA_BSRR = (1u << 16) | (1u << 17);
  }
  TIM2_CCER = 0u;
  TIM2_CCR1 = 0u;
  TIM2_CCR2 = 0u;
  TIM2_CR1 = 0u;
}

uint8_t servo_output_enabled(void)
{
  return (uint8_t)(VC_SERVO_OUTPUT_ENABLE ? 1u : 0u);
}

void servo_set_deg(uint8_t ch, int deg)
{
  if (!VC_SERVO_OUTPUT_ENABLE) return;
  if (ch != 1u) return;
  pulse_ch1(pitch_deg_to_us(deg));
}

void servo_set_pulse_us(uint8_t ch, uint16_t us)
{
  if (!VC_SERVO_OUTPUT_ENABLE) return;
  if (ch == 1u) {
    pulse_ch1(us);
  } else if (ch == 2u) {
    pulse_ch2(us);
  }
}
