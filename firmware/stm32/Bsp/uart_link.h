#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdint.h>

void uart_link_init(void);

/** IRQ / DMA path pushes raw RX bytes into the 512-byte ring (V0.3). */
void uart_link_rx_push(uint8_t byte);

/** Pop one byte from ring; return 1 if available. */
int uart_link_pop_byte(uint8_t *byte_out);

/** Legacy helper: returns 0 until IRQ path is wired; prefer pop_byte + protocol. */
int uart_link_poll_cmd(uint8_t *cmd);

#endif
