/**
 * Bring-up 双舵机手调 + OLED 表情轮播：
 * SERVO1=PA0/TIM2_CH1，SERVO2=PA1/TIM2_CH2，可同时开。
 * 命令（USART3 115200，经 ESP32 转发）：
 *   S1 <0..19999> / S1 OFF
 *   S2 <0..19999> / S2 OFF
 *   ALL OFF
 *   FACE <0..22> / NEXT / PREV
 *   AUTO <ms>     （0=停轮播）
 *   ANIM <0|1|2>  （blink / look_around / sleep_breathe）
 *   LIST / PING
 * 语音帧：AA 55 CMD 01 SEQ CHK ，CHK = CMD ^ 01 ^ SEQ
 *   云台只用已手调脉宽：抬头 500、低头 1000、俯仰中 750；
 *   偏航左 1000、中 1500、右 2000。电机不动。
 * PC 调试器：tools/oled_face_carousel.py
 */
#include <stdint.h>
#include "voice_car_oled_faces_rle_v1.h"

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_APB2ENR   REG32(0x40021018u)
#define RCC_APB1ENR   REG32(0x4002101Cu)
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

#define GPIOB_CRL     REG32(0x40010C00u)
#define GPIOB_IDR     REG32(0x40010C08u)
#define GPIOB_BSRR    REG32(0x40010C10u)
#define GPIOB_CRH     REG32(0x40010C04u)
#define GPIOC_CRH     REG32(0x40011004u)
#define GPIOC_ODR     REG32(0x4001100Cu)

#define USART3_SR     REG32(0x40004800u)
#define USART3_DR     REG32(0x40004804u)
#define USART3_BRR    REG32(0x40004808u)
#define USART3_CR1    REG32(0x4000480Cu)

#define STK_CTRL      REG32(0xE000E010u)
#define STK_LOAD      REG32(0xE000E014u)
#define STK_VAL       REG32(0xE000E018u)

#define RCC_IOPA      (1u << 2)
#define RCC_IOPB      (1u << 3)
#define RCC_IOPC      (1u << 4)
#define RCC_TIM2      (1u << 0)
#define RCC_USART3    (1u << 18)
#define USART_UE      (1u << 13)
#define USART_TE      (1u << 3)
#define USART_RE      (1u << 2)
#define USART_TXE     (1u << 7)
#define USART_RXNE    (1u << 5)
#define LED_PIN       13u

#define SERVO_PSC     7u
#define SERVO_ARR     19999u

volatile uint32_t g_ms;
volatile uint32_t g_beat;
static uint8_t g_oled_ok;
static uint8_t g_oled_addr = 0x3Cu;
static int g_servo1_us;
static int g_servo2_us;
static uint8_t g_servo1_on;
static uint8_t g_servo2_on;
static char g_rx_line[48];
static int g_rx_len;
static uint8_t g_vc_st;
static uint8_t g_vc_cmd;
static uint8_t g_vc_len;
static uint8_t g_vc_seq;
static uint8_t g_gim_kind; /* 0 无，1 摇头，2 点头 */
static uint8_t g_gim_phase;
static uint32_t g_gim_t;
static uint8_t g_face_id;
static uint16_t g_auto_ms;
static uint32_t g_auto_last;
static uint8_t g_anim_mode; /* 0=off 1=blink 2=look 3=sleep */
static uint8_t g_anim_idx;
static uint8_t g_face_fb[VC_OLED_BITMAP_BYTES];

static void uart_puts(const char *s);
static void put_u16(int v);

static void i2c_delay(void)
{
  for (volatile int i = 0; i < 30; ++i) {
  }
}

static void sda(int high)
{
  if (high) {
    GPIOB_BSRR = (1u << 7);
  } else {
    GPIOB_BSRR = (1u << (7 + 16));
  }
}

static void scl(int high)
{
  if (high) {
    GPIOB_BSRR = (1u << 6);
  } else {
    GPIOB_BSRR = (1u << (6 + 16));
  }
  i2c_delay();
}

static void i2c_start(void)
{
  sda(1);
  scl(1);
  sda(0);
  scl(0);
}

static void i2c_stop(void)
{
  sda(0);
  scl(1);
  sda(1);
  i2c_delay();
}

static int i2c_write(uint8_t byte)
{
  for (int i = 0; i < 8; ++i) {
    sda((byte & 0x80u) != 0u);
    scl(1);
    scl(0);
    byte <<= 1;
  }
  sda(1);
  scl(1);
  int ack = ((GPIOB_IDR >> 7) & 1u) == 0u;
  scl(0);
  return ack;
}

static int oled_cmds(uint8_t addr, const uint8_t *cmd, int len)
{
  i2c_start();
  if (!i2c_write((uint8_t)(addr << 1)) || !i2c_write(0x00u)) {
    i2c_stop();
    return 0;
  }
  for (int i = 0; i < len; ++i) {
    if (!i2c_write(cmd[i])) {
      i2c_stop();
      return 0;
    }
  }
  i2c_stop();
  return 1;
}

static int oled_data(uint8_t addr, const uint8_t *data, int len)
{
  i2c_start();
  if (!i2c_write((uint8_t)(addr << 1)) || !i2c_write(0x40u)) {
    i2c_stop();
    return 0;
  }
  for (int i = 0; i < len; ++i) {
    if (!i2c_write(data[i])) {
      i2c_stop();
      return 0;
    }
  }
  i2c_stop();
  return 1;
}

static void plot(uint8_t page[8][128], int x, int y)
{
  if ((unsigned)x >= 128u || (unsigned)y >= 64u) {
    return;
  }
  page[y / 8][x] |= (uint8_t)(1u << (y % 8));
}

static void glyph(uint8_t page[8][128], int x, int y, const uint8_t cols[5])
{
  for (int c = 0; c < 5; ++c) {
    for (int r = 0; r < 7; ++r) {
      if ((cols[c] & (1u << r)) == 0u) {
        continue;
      }
      for (int dy = 0; dy < 3; ++dy) {
        for (int dx = 0; dx < 3; ++dx) {
          plot(page, x + c * 3 + dx, y + r * 3 + dy);
        }
      }
    }
  }
}

static int oled_page(uint8_t addr, uint8_t page, uint8_t col, const uint8_t *data)
{
  uint8_t set[3];
  set[0] = (uint8_t)(0xB0u | page);
  set[1] = (uint8_t)(col & 0x0Fu);
  set[2] = (uint8_t)(0x10u | (col >> 4));
  if (!oled_cmds(addr, set, 3)) {
    return 0;
  }
  return oled_data(addr, data, 128);
}

static int oled_init(uint8_t addr)
{
  static const uint8_t init[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xFF, 0xD9, 0xF1,
    0xDB, 0x40, 0x8D, 0x14, 0xA4, 0xA6, 0xAF
  };
  return oled_cmds(addr, init, (int)sizeof(init));
}

static int oled_flush(uint8_t addr, uint8_t page[8][128])
{
  for (uint8_t col = 0; col <= 2; col = (uint8_t)(col + 2)) {
    for (uint8_t p = 0; p < 8; ++p) {
      if (!oled_page(addr, p, col, page[p])) {
        return 0;
      }
    }
  }
  return 1;
}

static const uint8_t F_MINUS[5] = {0x08, 0x1C, 0x1C, 0x1C, 0x08};
static const uint8_t F_P[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
static const uint8_t F_DIG[10][5] = {
  {0x3E, 0x51, 0x49, 0x45, 0x3E},
  {0x00, 0x42, 0x7F, 0x40, 0x00},
  {0x42, 0x61, 0x51, 0x49, 0x46},
  {0x21, 0x41, 0x45, 0x4B, 0x31},
  {0x18, 0x14, 0x12, 0x7F, 0x10},
  {0x27, 0x45, 0x45, 0x45, 0x39},
  {0x3C, 0x4A, 0x49, 0x49, 0x30},
  {0x01, 0x71, 0x09, 0x05, 0x03},
  {0x36, 0x49, 0x49, 0x49, 0x36},
  {0x06, 0x49, 0x49, 0x29, 0x1E}
};

static int oled_show_deg(uint8_t addr, int deg)
{
  uint8_t page[8][128];
  int a = deg;
  int neg = 0;
  int x = 20;

  if (a < 0) {
    neg = 1;
    a = -a;
  }
  if (a > 99) {
    a = 99;
  }

  for (int y = 0; y < 8; ++y) {
    for (int c = 0; c < 128; ++c) {
      page[y][c] = (y == 0 || y == 7) ? 0xFFu : 0x00u;
    }
  }

  glyph(page, 8, 16, F_P);
  if (neg) {
    glyph(page, 28, 16, F_MINUS);
    x = 48;
  }
  glyph(page, x, 16, F_DIG[(a / 10) % 10]);
  glyph(page, x + 20, 16, F_DIG[a % 10]);
  return oled_flush(addr, page);
}

static int oled_show_face(uint8_t id)
{
  uint8_t page[8][128];
  uint16_t i;

  if (!g_oled_ok) {
    return 0;
  }
  if (id >= VC_OLED_ASSET_COUNT) {
    return 0;
  }
  if (!vc_oled_decode((vc_oled_asset_id_t)id, g_face_fb, sizeof(g_face_fb))) {
    return 0;
  }
  for (i = 0; i < VC_OLED_BITMAP_BYTES; ++i) {
    ((uint8_t *)page)[i] = g_face_fb[i];
  }
  g_face_id = id;
  return oled_flush(g_oled_addr, page);
}

static void report_face(void)
{
  uart_puts("OK FACE ");
  put_u16(g_face_id);
  uart_puts("\r\n");
}

static void face_next(int delta)
{
  int id = (int)g_face_id + delta;
  if (id < 0) {
    id = (int)VC_OLED_ASSET_COUNT - 1;
  } else if (id >= (int)VC_OLED_ASSET_COUNT) {
    id = 0;
  }
  if (oled_show_face((uint8_t)id)) {
    report_face();
  } else {
    uart_puts("ERR FACE\r\n");
  }
}

static void anim_start(uint8_t which)
{
  /* which: 0 blink(11-14) 1 look(15-18) 2 sleep(19-22) */
  if (which > 2u) {
    uart_puts("ERR ANIM\r\n");
    return;
  }
  g_anim_mode = (uint8_t)(which + 1u);
  g_anim_idx = 0u;
  g_auto_ms = 180u;
  g_auto_last = g_ms;
  if (oled_show_face((uint8_t)(11u + which * 4u))) {
    uart_puts("OK ANIM ");
    put_u16(which);
    uart_puts("\r\n");
  } else {
    uart_puts("ERR ANIM\r\n");
  }
}

static void face_tick(void)
{
  uint8_t base;
  uint8_t id;

  if (g_auto_ms == 0u) {
    return;
  }
  if ((g_ms - g_auto_last) < g_auto_ms) {
    return;
  }
  g_auto_last = g_ms;

  if (g_anim_mode != 0u) {
    base = (uint8_t)(11u + (g_anim_mode - 1u) * 4u);
    g_anim_idx = (uint8_t)((g_anim_idx + 1u) & 3u);
    id = (uint8_t)(base + g_anim_idx);
    /* blink 不循环：到最后一帧后停在 blink_03 等价的睁眼(与 00 同 CRC 时可回 neutral) */
    if (g_anim_mode == 1u && g_anim_idx == 0u) {
      g_anim_mode = 0u;
      g_auto_ms = 0u;
      id = VC_FACE_NEUTRAL;
    }
    (void)oled_show_face(id);
    report_face();
    return;
  }

  face_next(1);
}

static void i2c_pins(void)
{
  uint32_t cr = GPIOB_CRL;
  GPIOB_BSRR = (1u << 6) | (1u << 7);
  cr &= ~((0xFu << 24) | (0xFu << 28));
  cr |= (0x6u << 24) | (0x6u << 28);
  GPIOB_CRL = cr;
}

void SysTick_Handler(void)
{
  g_ms++;
}

static void uart_putc(char c)
{
  while ((USART3_SR & USART_TXE) == 0u) {
  }
  USART3_DR = (uint8_t)c;
}

static void uart_puts(const char *s)
{
  while (*s) {
    uart_putc(*s++);
  }
}

static void put_u16(int v)
{
  char buf[8];
  char tmp[6];
  int n = 0;
  int i = 0;

  if (v <= 0) {
    uart_putc('0');
    return;
  }
  while (v > 0 && n < 5) {
    tmp[n++] = (char)('0' + (v % 10));
    v /= 10;
  }
  while (n > 0) {
    buf[i++] = tmp[--n];
  }
  buf[i] = '\0';
  uart_puts(buf);
}

static void tim2_base_on(void)
{
  TIM2_PSC = SERVO_PSC;
  TIM2_ARR = SERVO_ARR;
  TIM2_EGR = 1u;
  TIM2_CR1 = (1u << 7) | (1u << 0);
}

static void tim2_maybe_off(void)
{
  if (!g_servo1_on && !g_servo2_on) {
    TIM2_CCER = 0u;
    TIM2_CCR1 = 0u;
    TIM2_CCR2 = 0u;
    TIM2_CR1 = 0u;
  }
}

static void servo1_off(void)
{
  uint32_t cr;

  TIM2_CCER &= ~(1u << 0);
  TIM2_CCR1 = 0u;
  cr = GPIOA_CRL;
  cr &= ~(0xFu << 0);
  cr |= (0x4u << 0); /* PA0 悬空输入，避免信号线拉低让舵机一直响 */
  GPIOA_CRL = cr;
  g_servo1_on = 0u;
  g_servo1_us = 0;
  tim2_maybe_off();
}

static void servo2_off(void)
{
  uint32_t cr;

  TIM2_CCER &= ~(1u << 4);
  TIM2_CCR2 = 0u;
  cr = GPIOA_CRL;
  cr &= ~(0xFu << 4);
  cr |= (0x4u << 4); /* PA1 悬空输入 */
  GPIOA_CRL = cr;
  g_servo2_on = 0u;
  g_servo2_us = 0;
  tim2_maybe_off();
}

static void servo_all_off(void)
{
  servo1_off();
  servo2_off();
}

static void servo1_pulse(int us)
{
  uint32_t cr;
  uint32_t ccmr;
  uint32_t ccer;

  if (us < 0) {
    us = 0;
  }
  if (us > (int)SERVO_ARR) {
    us = (int)SERVO_ARR;
  }
  cr = GPIOA_CRL;
  cr &= ~(0xFu << 0);
  cr |= (0xBu << 0);
  GPIOA_CRL = cr;
  tim2_base_on();
  TIM2_CCR1 = (uint16_t)us;
  ccmr = TIM2_CCMR1;
  ccmr &= ~0xFFu;
  ccmr |= (0x6u << 4) | (1u << 3);
  TIM2_CCMR1 = ccmr;
  ccer = TIM2_CCER;
  ccer |= (1u << 0);
  TIM2_CCER = ccer;
  g_servo1_on = 1u;
  g_servo1_us = us;
}

static void servo2_pulse(int us)
{
  uint32_t cr;
  uint32_t ccmr;
  uint32_t ccer;

  if (us < 0) {
    us = 0;
  }
  if (us > (int)SERVO_ARR) {
    us = (int)SERVO_ARR;
  }
  cr = GPIOA_CRL;
  cr &= ~(0xFu << 4);
  cr |= (0xBu << 4);
  GPIOA_CRL = cr;
  tim2_base_on();
  TIM2_CCR2 = (uint16_t)us;
  ccmr = TIM2_CCMR1;
  ccmr &= ~0xFF00u;
  ccmr |= (0x6u << 12) | (1u << 11);
  TIM2_CCMR1 = ccmr;
  ccer = TIM2_CCER;
  ccer |= (1u << 4);
  TIM2_CCER = ccer;
  g_servo2_on = 1u;
  g_servo2_us = us;
}

static int parse_dec(const char *s, int *out)
{
  int v = 0;
  int n = 0;

  while (*s == ' ') {
    ++s;
  }
  if (*s < '0' || *s > '9') {
    return 0;
  }
  while (*s >= '0' && *s <= '9') {
    v = v * 10 + (*s - '0');
    ++s;
    ++n;
    if (n > 5) {
      return 0;
    }
  }
  while (*s == ' ') {
    ++s;
  }
  if (*s != '\0') {
    return 0;
  }
  *out = v;
  return 1;
}

static void show_pulse(int us)
{
  if (!g_oled_ok) {
    return;
  }
  oled_show_deg(g_oled_addr, us / 100);
}

static void report_state(void)
{
  uart_puts("OK S1 ");
  if (g_servo1_on) {
    put_u16(g_servo1_us);
  } else {
    uart_puts("OFF");
  }
  uart_puts(" S2 ");
  if (g_servo2_on) {
    put_u16(g_servo2_us);
  } else {
    uart_puts("OFF");
  }
  uart_puts("\r\n");
}

static int is_off_token(const char *arg)
{
  return (arg[0] == 'O' || arg[0] == 'o') && (arg[1] == 'F' || arg[1] == 'f') &&
         (arg[2] == 'F' || arg[2] == 'f') && arg[3] == '\0';
}

static int handle_servo_cmd(char which, const char *arg)
{
  int us;

  while (*arg == ' ') {
    ++arg;
  }
  if (is_off_token(arg)) {
    if (which == '1') {
      servo1_off();
      show_pulse(0);
    } else {
      servo2_off();
      show_pulse(0);
    }
    report_state();
    return 1;
  }
  if (parse_dec(arg, &us) && us >= 0 && us <= (int)SERVO_ARR) {
    if (which == '1') {
      servo1_pulse(us);
    } else {
      servo2_pulse(us);
    }
    show_pulse(us);
    report_state();
    return 1;
  }
  return 0;
}

static void handle_line(const char *line)
{
  int v;

  if (line[0] == 'P' && line[1] == 'I' && line[2] == 'N' && line[3] == 'G' &&
      line[4] == '\0') {
    uart_puts("PONG\r\n");
    return;
  }
  if (line[0] == 'L' && line[1] == 'I' && line[2] == 'S' && line[3] == 'T' &&
      line[4] == '\0') {
    uart_puts("OK LIST ");
    put_u16((int)VC_OLED_ASSET_COUNT);
    uart_puts(" CUR ");
    put_u16(g_face_id);
    uart_puts("\r\n");
    return;
  }
  if (line[0] == 'N' && line[1] == 'E' && line[2] == 'X' && line[3] == 'T' &&
      line[4] == '\0') {
    g_anim_mode = 0u;
    face_next(1);
    return;
  }
  if (line[0] == 'P' && line[1] == 'R' && line[2] == 'E' && line[3] == 'V' &&
      line[4] == '\0') {
    g_anim_mode = 0u;
    face_next(-1);
    return;
  }
  if (line[0] == 'F' && line[1] == 'A' && line[2] == 'C' && line[3] == 'E' &&
      (line[4] == ' ' || line[4] == '\0')) {
    const char *arg = line + 4;
    while (*arg == ' ') {
      ++arg;
    }
    if (parse_dec(arg, &v) && v >= 0 && v < (int)VC_OLED_ASSET_COUNT) {
      g_anim_mode = 0u;
      g_auto_ms = 0u;
      if (oled_show_face((uint8_t)v)) {
        report_face();
      } else {
        uart_puts("ERR FACE\r\n");
      }
      return;
    }
  }
  if (line[0] == 'A' && line[1] == 'U' && line[2] == 'T' && line[3] == 'O' &&
      (line[4] == ' ' || line[4] == '\0')) {
    const char *arg = line + 4;
    while (*arg == ' ') {
      ++arg;
    }
    if (parse_dec(arg, &v) && v >= 0 && v <= 5000) {
      g_anim_mode = 0u;
      g_auto_ms = (uint16_t)v;
      g_auto_last = g_ms;
      uart_puts("OK AUTO ");
      put_u16(v);
      uart_puts("\r\n");
      return;
    }
  }
  if (line[0] == 'A' && line[1] == 'N' && line[2] == 'I' && line[3] == 'M' &&
      (line[4] == ' ' || line[4] == '\0')) {
    const char *arg = line + 4;
    while (*arg == ' ') {
      ++arg;
    }
    if (parse_dec(arg, &v) && v >= 0 && v <= 2) {
      anim_start((uint8_t)v);
      return;
    }
  }
  if (line[0] == 'A' && line[1] == 'L' && line[2] == 'L' &&
      (line[3] == ' ' || line[3] == '\0')) {
    const char *arg = line + 3;
    while (*arg == ' ') {
      ++arg;
    }
    if (is_off_token(arg) || arg[0] == '\0') {
      servo_all_off();
      show_pulse(0);
      report_state();
      return;
    }
  }
  if ((line[0] == 'S') && (line[1] == '1' || line[1] == '2') &&
      (line[2] == ' ' || line[2] == '\0')) {
    if (handle_servo_cmd(line[1], line + 2)) {
      return;
    }
  }
  uart_puts("ERR\r\n");
}

static void face_hold(uint8_t id)
{
  g_anim_mode = 0u;
  g_auto_ms = 0u;
  (void)oled_show_face(id);
}

static void gimbal_tick(void)
{
  if (g_gim_kind == 0u) {
    return;
  }
  if ((g_ms - g_gim_t) < 450u) {
    return;
  }
  g_gim_t = g_ms;
  g_gim_phase++;
  if (g_gim_kind == 1u) {
    if (g_gim_phase == 1u) {
      servo2_pulse(2000);
    } else if (g_gim_phase == 2u) {
      servo2_pulse(1000);
    } else {
      servo2_pulse(1500);
      g_gim_kind = 0u;
      face_hold(VC_FACE_NEUTRAL);
    }
  } else if (g_gim_phase == 1u) {
    servo1_pulse(500); /* 抬头 -45° */
  } else {
    servo1_pulse(750);
    g_gim_kind = 0u;
    face_hold(VC_FACE_NEUTRAL);
  }
  report_state();
}

/* 只使用 board.h 里已手调的脉宽。 */
static void gimbal_on_vc(uint8_t cmd)
{
  g_gim_kind = 0u;
  if (cmd != 0x16u && cmd >= 0x10u && cmd <= 0x17u) {
    face_hold(VC_FACE_NEUTRAL);
  }
  switch (cmd) {
    case 0x10u:
      servo1_pulse(500); /* 抬头 -45° */
      break;
    case 0x11u:
      servo1_pulse(1000); /* 低头 0° */
      break;
    case 0x14u:
      servo1_pulse(750);
      break;
    case 0x12u:
      servo2_pulse(1000);
      break;
    case 0x13u:
      servo2_pulse(2000);
      break;
    case 0x15u:
      servo2_pulse(1500);
      break;
    case 0x16u:
      g_gim_kind = 1u;
      g_gim_phase = 0u;
      g_gim_t = g_ms;
      servo2_pulse(1000);
      anim_start(1u); /* 左右看，和摇头一起 */
      break;
    case 0x17u:
      g_gim_kind = 2u;
      g_gim_phase = 0u;
      g_gim_t = g_ms;
      servo1_pulse(1000); /* 低头 */
      face_hold(VC_FACE_LISTENING);
      break;
    default:
      break;
  }
  report_state();
}

static void put_hex8(uint8_t v)
{
  static const char k_hex[] = "0123456789ABCDEF";
  char s[3];

  s[0] = k_hex[v >> 4];
  s[1] = k_hex[v & 0x0Fu];
  s[2] = '\0';
  uart_puts(s);
}

/* 吃掉一字节。返回 1 表示该字节属于语音帧，不要再进文本行。 */
static int feed_vc(uint8_t b)
{
  switch (g_vc_st) {
    case 0u:
      if (b == 0xAAu) {
        g_vc_st = 1u;
        g_rx_len = 0;
        return 1;
      }
      return 0;
    case 1u:
      if (b == 0x55u) {
        g_vc_st = 2u;
        return 1;
      }
      g_vc_st = (b == 0xAAu) ? 1u : 0u;
      return 1;
    case 2u:
      g_vc_cmd = b;
      g_vc_st = 3u;
      return 1;
    case 3u:
      g_vc_len = b;
      g_vc_st = 4u;
      return 1;
    case 4u:
      g_vc_seq = b;
      g_vc_st = 5u;
      return 1;
    case 5u:
      g_vc_st = 0u;
      if (g_vc_len == 1u && b == (uint8_t)(g_vc_cmd ^ g_vc_len ^ g_vc_seq)) {
        uart_puts("VC ");
        put_hex8(g_vc_cmd);
        uart_puts("\r\n");
        gimbal_on_vc(g_vc_cmd);
      } else {
        uart_puts("VC BAD\r\n");
      }
      return 1;
    default:
      g_vc_st = 0u;
      return 0;
  }
}

static void poll_uart(void)
{
  while ((USART3_SR & USART_RXNE) != 0u) {
    char c = (char)(USART3_DR & 0xFFu);
    if (feed_vc((uint8_t)c)) {
      continue;
    }
    if (c == '\n' || c == '\r') {
      if (g_rx_len > 0) {
        g_rx_line[g_rx_len] = '\0';
        handle_line(g_rx_line);
        g_rx_len = 0;
      }
    } else if (g_rx_len < (int)sizeof(g_rx_line) - 1) {
      g_rx_line[g_rx_len++] = c;
    } else {
      g_rx_len = 0;
    }
  }
}

static void board_init(void)
{
  uint32_t cr;

  RCC_APB2ENR |= RCC_IOPA | RCC_IOPB | RCC_IOPC;
  RCC_APB1ENR |= RCC_TIM2 | RCC_USART3;

  cr = GPIOB_CRH;
  cr &= ~((0xFu << 8) | (0xFu << 12));
  cr |= (0xBu << 8) | (0x4u << 12);
  GPIOB_CRH = cr;

  cr = GPIOC_CRH;
  cr &= ~(0xFu << 20);
  cr |= (0x3u << 20);
  GPIOC_CRH = cr;
  GPIOC_ODR |= (1u << LED_PIN);

  i2c_pins();
  servo_all_off();

  USART3_BRR = 0x45u;
  USART3_CR1 = USART_UE | USART_TE | USART_RE;

  STK_LOAD = 8000u - 1u;
  STK_VAL = 0u;
  STK_CTRL = 0x7u;
}

int main(void)
{
  uint32_t last = 0;

  board_init();
  servo_all_off();
  g_rx_len = 0;
  while (g_ms < 100u) {
  }

  if (oled_init(0x3C) && oled_show_deg(0x3C, 0)) {
    g_oled_ok = 1u;
    g_oled_addr = 0x3Cu;
  } else if (oled_init(0x3D) && oled_show_deg(0x3D, 0)) {
    g_oled_ok = 1u;
    g_oled_addr = 0x3Du;
  }

  g_face_id = 0u;
  g_auto_ms = 0u;
  g_anim_mode = 0u;
  if (g_oled_ok) {
    (void)oled_show_face(VC_FACE_NEUTRAL);
  }

  uart_puts("READY S1 S2 FACE\r\n");
  report_state();
  report_face();
  last = g_ms;

  for (;;) {
    poll_uart();
    gimbal_tick();
    face_tick();
    if ((g_ms - last) >= 1000u) {
      last += 1000u;
      g_beat++;
      if ((g_beat % 5u) == 0u) {
        report_state();
      }
    }
  }
}
