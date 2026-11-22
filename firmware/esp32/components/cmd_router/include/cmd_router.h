#pragma once

#include <stdint.h>
#include "cmd_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void cmd_router_init(void);
/** Voice or remote submit a normalized command. */
void cmd_router_submit(vc_cmd_t cmd);
void cmd_router_poll(void);

#ifdef __cplusplus
}
#endif
