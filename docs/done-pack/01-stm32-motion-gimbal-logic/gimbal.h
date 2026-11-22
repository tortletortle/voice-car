#ifndef GIMBAL_H
#define GIMBAL_H

#include "../../common/cmd_protocol.h"

void gimbal_init(void);
void gimbal_on_cmd(vc_cmd_t cmd);

#endif
