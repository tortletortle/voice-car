#include <assert.h>
#include <stdio.h>
#include "remote_actions.h"

int main(void)
{
    static const struct {
        const char *name;
        vc_cmd_t command;
    } cases[] = {
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
    vc_remote_action_t result;
    unsigned i;

    for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        assert(vc_remote_action_lookup(cases[i].name, &result));
        assert(result.command == cases[i].command);
        assert(vc_cmd_is_known((uint8_t)result.command));
    }
    assert(!vc_remote_action_lookup("delete_all_files", &result));
    assert(!vc_remote_action_lookup("", &result));
    puts("remote action allow-list tests: PASS");
    return 0;
}
