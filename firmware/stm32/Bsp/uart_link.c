#include "uart_link.h"
#include "board.h"

/*
 * V0.3 design: USART3 IRQ fills a ring so OLED page writes cannot starve RX.
 * Hardware pins (local): PB10 TX / PB11 RX @ LINK_UART_BAUD.
 * Until Keil wires USART3 IRQ39, push() is unused and pop returns empty.
 */

static uint8_t s_ring[LINK_RX_RING_SIZE];
static volatile uint16_t s_head;
static volatile uint16_t s_tail;

void uart_link_init(void)
{
  s_head = 0u;
  s_tail = 0u;
  (void)LINK_UART_BAUD;
  (void)LINK_TX_PIN_PB;
  (void)LINK_RX_PIN_PB;
  /* TODO: RCC + GPIOB + USART3 + NVIC IRQ39 → uart_link_rx_push(DR). */
}

void uart_link_rx_push(uint8_t byte)
{
  uint16_t next = (uint16_t)((s_head + 1u) % LINK_RX_RING_SIZE);
  if (next == s_tail) {
    /* Drop oldest on overrun to keep newest stream. */
    s_tail = (uint16_t)((s_tail + 1u) % LINK_RX_RING_SIZE);
  }
  s_ring[s_head] = byte;
  s_head = next;
}

int uart_link_pop_byte(uint8_t *byte_out)
{
  if (!byte_out) return 0;
  if (s_tail == s_head) return 0;
  *byte_out = s_ring[s_tail];
  s_tail = (uint16_t)((s_tail + 1u) % LINK_RX_RING_SIZE);
  return 1;
}

int uart_link_poll_cmd(uint8_t *cmd)
{
  (void)cmd;
  return 0;
}
