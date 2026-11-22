#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

void servo_init(void);
/** Positional degrees (SERVO1 pitch: 0=低头, -45=抬头). Gated by VC_SERVO_OUTPUT_ENABLE. */
void servo_set_deg(uint8_t ch, int deg);
/** Pulse µs (SERVO2 yaw positional: L1000 / C1500 / R2000). */
void servo_set_pulse_us(uint8_t ch, uint16_t us);
uint8_t servo_output_enabled(void);

#endif
