#pragma once

#include "cmd_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void link_stm32_init(void);
void link_stm32_send(vc_cmd_t cmd);

#ifdef __cplusplus
}
#endif
