/**
 * STM32F103C8T6 bring-up.
 * Clock: HSI 8 MHz (internal). Do not wait for the external crystal.
 * UART: USART3, PB10 TX / PB11 RX, 115200 8N1.
 *       PCB U1.35 = PB10 = net ESP32_RX, U1.36 = PB11 = net ESP32_TX.
 *       BRR = 0x45. Actual baud 8000000/69 ≈ 115942, error about 0.6%.
 * LED: core-board PC13, push-pull, held high (on) the whole time.
 * OLED: 0.96 inch I2C, PB6 SCL / PB7 SDA (U1.14 / U1.15). Address 0x3C, else 0x3D.
 * Servo: stopped. TIM2 off, PA0/PA1 floating. User researching drive.
 */
#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_APB2ENR   REG32(0x40021018u)
#define RCC_APB1ENR   REG32(0x4002101Cu)
#define GPIOA_CRL     REG32(0x40010800u)
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
#define LED_PIN       13u

volatile uint32_t g_ms;
volatile uint32_t g_beat;
static uint8_t g_oled_ok;

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

static void plot(uint8_t page[3][128], int x, int y)
{
  if ((unsigned)x >= 128u || (unsigned)y >= 64u) {
    return;
  }
  page[y / 8][x] |= (uint8_t)(1u << (y % 8));
}

static void glyph(uint8_t page[3][128], int x, int y, const uint8_t cols[5])
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

static int oled_show_ok(uint8_t addr)
{
  /* Page/column addressing. SH1106 ignores SSD1306's 0x21/0x22 mode. */
  static const uint8_t init[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xFF, 0xD9, 0xF1,
    0xDB, 0x40, 0x8D, 0x14, 0xA4, 0xA6, 0xAF
  };
  static const uint8_t ok_o[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
  static const uint8_t ok_k[5] = {0x7F, 0x08, 0x14, 0x22, 0x41};
  uint8_t page[8][128];

  if (!oled_cmds(addr, init, (int)sizeof(init))) {
    return 0;
  }
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 128; ++x) {
      page[y][x] = (y == 0 || y == 7) ? 0xFFu : 0x00u;
    }
  }
  glyph((uint8_t (*)[128])page, 40, 16, ok_o);
  glyph((uint8_t (*)[128])page, 64, 16, ok_k);

  /* Column 0, then column 2: covers SSD1306 and SH1106 glass offset. */
  for (uint8_t col = 0; col <= 2; col = (uint8_t)(col + 2)) {
    for (uint8_t p = 0; p < 8; ++p) {
      if (!oled_page(addr, p, col, page[p])) {
        return 0;
      }
    }
  }
  return 1;
}

static void i2c_pins(void)
{
  uint32_t cr = GPIOB_CRL;
  GPIOB_BSRR = (1u << 6) | (1u << 7);
  cr &= ~((0xFu << 24) | (0xFu << 28));
  cr |= (0x6u << 24) | (0x6u << 28); /* PB6/PB7 open-drain, 2 MHz */
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

static void servo_stop(void)
{
  uint32_t cr = GPIOA_CRL;

  TIM2_CR1 = 0u;
  TIM2_CCER = 0u;
  TIM2_CCR1 = 0u;
  TIM2_CCR2 = 0u;

  cr &= ~((0xFu << 0) | (0xFu << 4));
  cr |= (0x4u << 0) | (0x4u << 4); /* PA0/PA1 floating input */
  GPIOA_CRL = cr;
}

static void board_init(void)
{
  uint32_t cr;

  RCC_APB2ENR |= RCC_IOPA | RCC_IOPB | RCC_IOPC;
  RCC_APB1ENR |= RCC_TIM2 | RCC_USART3;

  cr = GPIOB_CRH;
  cr &= ~((0xFu << 8) | (0xFu << 12));
  cr |= (0xBu << 8);   /* PB10: AF push-pull, 50 MHz, USART3_TX */
  cr |= (0x4u << 12);  /* PB11: floating input, USART3_RX */
  GPIOB_CRH = cr;

  cr = GPIOC_CRH;
  cr &= ~(0xFu << 20);
  cr |= (0x3u << 20);  /* PC13: push-pull output, 50 MHz */
  GPIOC_CRH = cr;
  GPIOC_ODR |= (1u << LED_PIN); /* high = LED on */

  i2c_pins();
  servo_stop();

  USART3_BRR = 0x45u;
  USART3_CR1 = USART_UE | USART_TE | USART_RE;

  STK_LOAD = 8000u - 1u;
  STK_VAL = 0u;
  STK_CTRL = 0x7u; /* AHB clock, interrupt, enable */
}

int main(void)
{
  uint32_t last = 0;

  board_init();
  while (g_ms < 100u) {
  }
  if (oled_show_ok(0x3C) || oled_show_ok(0x3D)) {
    g_oled_ok = 1u;
  }
  uart_puts(g_oled_ok ? "OLED OK\r\n" : "OLED NO\r\n");
  uart_puts("SERVO OFF\r\n");

  for (;;) {
    uint32_t now = g_ms;
    if ((now - last) < 500u) {
      continue;
    }
    last += 500u;
    g_beat++;
    uart_puts("LINK\r\n");
    if ((g_beat % 4u) == 0u) {
      uart_puts(g_oled_ok ? "OLED OK\r\n" : "OLED NO\r\n");
      uart_puts("SERVO OFF\r\n");
    }
  }
}
