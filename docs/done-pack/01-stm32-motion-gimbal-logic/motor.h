#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

typedef enum {
  MOTOR_STOP = 0,
  MOTOR_FWD,
  MOTOR_BACK,
} motor_dir_t;

void motor_init(void);
/** STBY enable/disable (TB6612-style). */
void motor_stby(int enable);
void motor_set(motor_dir_t left, motor_dir_t right, uint8_t pwm_percent);
void motor_all_stop(void);

#endif
