#ifndef VOICE_CAR_CMD_PROTOCOL_H
#define VOICE_CAR_CMD_PROTOCOL_H

#include <stdint.h>

/* ESP32-S3 -> STM32F103, fixed v0 frame: AA 55 CMD LEN SEQ CHK. */
#define VC_FRAME_HEAD0 0xAAu
#define VC_FRAME_HEAD1 0x55u
#define VC_FRAME_LEN   6u
#define VC_FRAME_V0_LEN_FIELD 1u

typedef enum {
  CMD_NOP                  = 0x00,
  CMD_STOP                 = 0x01,
  CMD_FWD                  = 0x02,
  CMD_BACK                 = 0x03,
  CMD_LEFT                 = 0x04,
  CMD_RIGHT                = 0x05,
  CMD_GIMBAL_UP            = 0x10,
  CMD_GIMBAL_DOWN          = 0x11,
  CMD_GIMBAL_LEFT          = 0x12,
  CMD_GIMBAL_RIGHT         = 0x13,
  CMD_GIMBAL_PITCH_CENTER  = 0x14,
  CMD_GIMBAL_YAW_STOP      = 0x15,
  CMD_GIMBAL_NOD           = 0x16,
  CMD_GIMBAL_SHAKE         = 0x17,
  CMD_GIMBAL_NOD_TWICE     = 0x18,
  CMD_GIMBAL_SHAKE_TWICE   = 0x19,
  CMD_GIMBAL_NEUTRAL       = 0x1A,

  CMD_EXPR_NORMAL          = 0x20,
  CMD_EXPR_HAPPY           = 0x21,
  CMD_EXPR_LISTENING       = 0x22,
  CMD_EXPR_THINKING        = 0x23,
  CMD_EXPR_SAD             = 0x24,
  CMD_EXPR_ANGRY           = 0x25,
  CMD_EXPR_SLEEP           = 0x26,
  CMD_EXPR_CUTE            = 0x27,
  CMD_EXPR_FOCUS           = 0x28,
  CMD_EXPR_CURIOUS         = 0x29,

  /* Semantic aliases share the same physical action and wire ID. */
  CMD_GIMBAL_AGREE         = CMD_GIMBAL_NOD,
  CMD_GIMBAL_DISAGREE      = CMD_GIMBAL_SHAKE,
  CMD_GIMBAL_GREETING      = CMD_GIMBAL_NOD_TWICE,
  CMD_GIMBAL_HOME          = CMD_GIMBAL_NEUTRAL,
  /* Source compatibility: old CENTER means pitch-center only. */
  CMD_GIMBAL_CENTER        = CMD_GIMBAL_PITCH_CENTER
} vc_cmd_t;

static inline uint8_t vc_checksum(uint8_t cmd, uint8_t len, uint8_t seq)
{
  return (uint8_t)(cmd ^ len ^ seq);
}

static inline uint8_t vc_cmd_is_motion(uint8_t cmd)
{
  switch (cmd) {
    case CMD_STOP:
    case CMD_FWD:
    case CMD_BACK:
    case CMD_LEFT:
    case CMD_RIGHT:
      return 1u;
    default:
      return 0u;
  }
}

static inline uint8_t vc_cmd_is_gimbal(uint8_t cmd)
{
  switch (cmd) {
    case CMD_GIMBAL_UP:
    case CMD_GIMBAL_DOWN:
    case CMD_GIMBAL_LEFT:
    case CMD_GIMBAL_RIGHT:
    case CMD_GIMBAL_PITCH_CENTER:
    case CMD_GIMBAL_YAW_STOP:
    case CMD_GIMBAL_NOD:
    case CMD_GIMBAL_SHAKE:
    case CMD_GIMBAL_NOD_TWICE:
    case CMD_GIMBAL_SHAKE_TWICE:
    case CMD_GIMBAL_NEUTRAL:
      return 1u;
    default:
      return 0u;
  }
}

static inline uint8_t vc_cmd_is_expression(uint8_t cmd)
{
  switch (cmd) {
    case CMD_EXPR_NORMAL:
    case CMD_EXPR_HAPPY:
    case CMD_EXPR_LISTENING:
    case CMD_EXPR_THINKING:
    case CMD_EXPR_SAD:
    case CMD_EXPR_ANGRY:
    case CMD_EXPR_SLEEP:
    case CMD_EXPR_CUTE:
    case CMD_EXPR_FOCUS:
    case CMD_EXPR_CURIOUS:
      return 1u;
    default:
      return 0u;
  }
}

static inline uint8_t vc_cmd_is_known(uint8_t cmd)
{
  return (uint8_t)(cmd == CMD_NOP || vc_cmd_is_motion(cmd) ||
                   vc_cmd_is_gimbal(cmd) || vc_cmd_is_expression(cmd));
}

static inline void vc_pack(uint8_t cmd, uint8_t seq, uint8_t out[VC_FRAME_LEN])
{
  const uint8_t len = VC_FRAME_V0_LEN_FIELD;
  out[0] = VC_FRAME_HEAD0;
  out[1] = VC_FRAME_HEAD1;
  out[2] = cmd;
  out[3] = len;
  out[4] = seq;
  out[5] = vc_checksum(cmd, len, seq);
}

#endif
