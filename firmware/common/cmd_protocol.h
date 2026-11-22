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
  CMD_GIMBAL_YAW_STOP      = 0x15, /* SERVO2 回中 1500µs */
  CMD_GIMBAL_YAW_SHAKE     = 0x16, /* SERVO2 摇头：左→右→左→回中 */
  CMD_GIMBAL_PITCH_NOD     = 0x17, /* SERVO1 点头：低头→抬头→回中 */
  /* 表情：与 CI1302 V00874 识别 ID 同值。资源号见 vc_cmd_face_asset。 */
  CMD_FACE_HELLO           = 0x20, /* 打个招呼 → happy */
  CMD_FACE_HAPPY           = 0x21, /* 你真好看 / 讲个笑话 → happy */
  CMD_FACE_CUTE            = 0x22, /* 你真可爱 / 你是谁呀 → cute */
  CMD_FACE_ANGRY           = 0x23, /* 我生气了 → angry */
  CMD_FACE_SLEEP           = 0x24, /* 我想睡觉 → sleep */
  CMD_FACE_LISTEN          = 0x25, /* 我在听呢 → listening */
  CMD_FACE_CONFUSED        = 0x26, /* 我没听懂 → confused */
  CMD_FACE_SURPRISE        = 0x27, /* 吓我一跳 → surprised */
  CMD_FACE_SAD             = 0x28, /* 我很难过 → sad */
  CMD_FACE_WINK            = 0x29, /* 眨一下眼 / 我喜欢你 → wink */
  CMD_FACE_REJECT          = 0x2A, /* 这个不行 → reject */
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
    case CMD_GIMBAL_YAW_SHAKE:
    case CMD_GIMBAL_PITCH_NOD:
    case CMD_FACE_HELLO:
    case CMD_FACE_HAPPY:
    case CMD_FACE_CUTE:
    case CMD_FACE_ANGRY:
    case CMD_FACE_SLEEP:
    case CMD_FACE_LISTEN:
    case CMD_FACE_CONFUSED:
    case CMD_FACE_SURPRISE:
    case CMD_FACE_SAD:
    case CMD_FACE_WINK:
    case CMD_FACE_REJECT:
      return 1u;
    default:
      return 0u;
  }
}

/* OLED pack V1.0 静态脸：1 happy … 10 reject。非表情命令返回 0。 */
static inline uint8_t vc_cmd_face_asset(uint8_t cmd, uint8_t *asset_out)
{
  static const uint8_t k_asset[] = {
    1u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u
  };
  if (cmd < (uint8_t)CMD_FACE_HELLO || cmd > (uint8_t)CMD_FACE_REJECT) {
    return 0u;
  }
  if (asset_out) {
    *asset_out = k_asset[cmd - (uint8_t)CMD_FACE_HELLO];
  }
  return 1u;
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
