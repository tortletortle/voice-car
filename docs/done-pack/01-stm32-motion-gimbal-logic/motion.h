#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>
#include "../../common/cmd_protocol.h"

void motion_init(void);
void motion_on_cmd(vc_cmd_t cmd);
/** Call every 1 ms (SysTick) or from main loop with elapsed ms. */
void motion_tick(uint32_t elapsed_ms);

#endif
