/**
 * Shared command IDs: ESP32 (router) -> STM32 (executor).
 * Voice word tables map into these later; values are placeholders.
 *
 * Wire frame (v0): AA 55 CMD LEN SEQ CHK
 *   CHK = CMD ^ LEN ^ SEQ (simple XOR, replace later if needed)
 */
#ifndef VOICE_CAR_CMD_PROTOCOL_H
#define VOICE_CAR_CMD_PROTOCOL_H

#include <stdint.h>

#define VC_FRAME_HEAD0 0xAAu
#define VC_FRAME_HEAD1 0x55u
#define VC_FRAME_LEN   6u

typedef enum {
  CMD_NOP            = 0x00,
  CMD_STOP           = 0x01,
  CMD_FWD            = 0x02,
  CMD_BACK           = 0x03,
  CMD_LEFT           = 0x04,
  CMD_RIGHT          = 0x05,
  CMD_GIMBAL_UP      = 0x10,
  CMD_GIMBAL_DOWN    = 0x11,
  CMD_GIMBAL_LEFT    = 0x12,
  CMD_GIMBAL_RIGHT   = 0x13,
  CMD_GIMBAL_CENTER  = 0x14,
} vc_cmd_t;

static inline uint8_t vc_checksum(uint8_t cmd, uint8_t len, uint8_t seq)
{
  return (uint8_t)(cmd ^ len ^ seq);
}

/** Pack 6-byte frame into out[6]. */
static inline void vc_pack(uint8_t cmd, uint8_t seq, uint8_t out[VC_FRAME_LEN])
{
  const uint8_t len = 1u; /* payload bytes after LEN field: none beyond CMD for v0 */
  out[0] = VC_FRAME_HEAD0;
  out[1] = VC_FRAME_HEAD1;
  out[2] = cmd;
  out[3] = len;
  out[4] = seq;
  out[5] = vc_checksum(cmd, len, seq);
}

#endif /* VOICE_CAR_CMD_PROTOCOL_H */
