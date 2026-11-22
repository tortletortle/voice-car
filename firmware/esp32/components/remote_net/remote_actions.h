#ifndef VOICE_CAR_REMOTE_ACTIONS_H
#define VOICE_CAR_REMOTE_ACTIONS_H

#include <stdbool.h>
#include "cmd_protocol.h"

typedef struct {
    vc_cmd_t command;
} vc_remote_action_t;

bool vc_remote_action_lookup(const char *name, vc_remote_action_t *out);

#endif
