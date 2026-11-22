#include "remote_actions.h"

#include <stddef.h>
#include <string.h>

typedef struct {
    const char *name;
    vc_cmd_t command;
} action_entry_t;

static const action_entry_t ACTIONS[] = {
    {"stop", CMD_STOP},
    {"forward", CMD_FWD},
    {"backward", CMD_BACK},
    {"turn_left", CMD_LEFT},
    {"turn_right", CMD_RIGHT},
    {"pitch_up", CMD_GIMBAL_UP},
    {"pitch_down", CMD_GIMBAL_DOWN},
    {"pitch_center", CMD_GIMBAL_PITCH_CENTER},
    {"yaw_left", CMD_GIMBAL_LEFT},
    {"yaw_right", CMD_GIMBAL_RIGHT},
    {"yaw_stop", CMD_GIMBAL_YAW_STOP},
    {"nod", CMD_GIMBAL_NOD},
    {"shake", CMD_GIMBAL_SHAKE},
    {"nod_twice", CMD_GIMBAL_NOD_TWICE},
    {"shake_twice", CMD_GIMBAL_SHAKE_TWICE},
    {"head_neutral", CMD_GIMBAL_NEUTRAL},
    {"agree", CMD_GIMBAL_AGREE},
    {"disagree", CMD_GIMBAL_DISAGREE},
    {"hello", CMD_GIMBAL_GREETING},
    {"greeting", CMD_GIMBAL_GREETING}
};

bool vc_remote_action_lookup(const char *name, vc_remote_action_t *out)
{
    size_t i;
    if (name == NULL || out == NULL) return false;
    for (i = 0u; i < sizeof(ACTIONS) / sizeof(ACTIONS[0]); ++i) {
        if (strcmp(name, ACTIONS[i].name) == 0) {
            out->command = ACTIONS[i].command;
            return vc_cmd_is_known((uint8_t)out->command) != 0u;
        }
    }
    return false;
}
