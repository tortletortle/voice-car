#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

void servo_init(void);
void servo_set_deg(uint8_t ch, uint8_t deg);

#endif
