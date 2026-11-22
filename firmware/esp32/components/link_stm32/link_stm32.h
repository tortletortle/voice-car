#ifndef VOICE_CAR_LINK_STM32_H
#define VOICE_CAR_LINK_STM32_H

#include <stdbool.h>
#include "cmd_protocol.h"
#include "esp_err.h"

esp_err_t link_stm32_init(void);
bool link_stm32_send(vc_cmd_t cmd);

#endif
