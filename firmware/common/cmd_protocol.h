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
  /* Source compatibility: old CENTER now means pitch-center only. */
  CMD_GIMBAL_CENTER        = CMD_GIMBAL_PITCH_CENTER
} vc_cmd_t;

static inline uint8_t vc_checksum(uint8_t cmd, uint8_t len, uint8_t seq)
{
  return (uint8_t)(cmd ^ len ^ seq);
}

static inline uint8_t vc_cmd_is_known(uint8_t cmd)
{
  switch (cmd) {
    case CMD_NOP:
    case CMD_STOP:
    case CMD_FWD:
    case CMD_BACK:
    case CMD_LEFT:
    case CMD_RIGHT:
    case CMD_GIMBAL_UP:
    case CMD_GIMBAL_DOWN:
    case CMD_GIMBAL_LEFT:
    case CMD_GIMBAL_RIGHT:
    case CMD_GIMBAL_PITCH_CENTER:
    case CMD_GIMBAL_YAW_STOP:
      return 1u;
    default:
      return 0u;
  }
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
