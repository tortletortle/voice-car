#ifndef VOICE_CAR_CMD_ROUTER_H
#define VOICE_CAR_CMD_ROUTER_H

#include <stdbool.h>
#include "cmd_protocol.h"
#include "esp_err.h"

esp_err_t cmd_router_init(void);
/* Returns true when the shared router accepts the command. */
bool cmd_router_submit(vc_cmd_t cmd);

#endif
