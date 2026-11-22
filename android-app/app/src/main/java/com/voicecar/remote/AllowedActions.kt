package com.voicecar.remote

/**
 * Actions that exist in the shared Voice Car command contract.
 * UI options must not invent actions that the firmware cannot dispatch.
 */
object AllowedActions {
    val values = setOf(
        "stop", "forward", "backward", "turn_left", "turn_right",
        "pitch_up", "pitch_down", "pitch_center",
        "yaw_left", "yaw_right", "yaw_stop",
        "nod", "shake", "nod_twice", "shake_twice", "head_neutral",
        "agree", "disagree", "hello", "greeting"
    )
}
