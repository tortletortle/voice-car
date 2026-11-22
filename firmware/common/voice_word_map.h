#ifndef VOICE_CAR_VOICE_WORD_MAP_H
#define VOICE_CAR_VOICE_WORD_MAP_H

#include <stdint.h>
#include "cmd_protocol.h"

/*
 * CI1302 / Module ASR ID → vc_cmd_t (V0.3).
 * ESP GPIO17 RX / GPIO18 TX (VOICE_RX/TX). Frame: AA 55 00 ID FB @ 115200.
 * ID 与已烧的 V00681（UART1 115200）readme 第 4 字节一致，也与 V00874 相同。
 *
 * 优先：云台可见动作（抬头/低头/摇头/点头），其次表情包静态脸。
 * asr_id 与 V00874 readme 的第 4 字节一致，表情命令值与 ID 相同。
 */

typedef struct {
  uint8_t asr_id;
  vc_cmd_t cmd;
} vc_voice_map_entry_t;

static const vc_voice_map_entry_t VC_VOICE_WORD_MAP[] = {
  /* 舵机 / 云台（先看得到） */
  { 0x10u, CMD_GIMBAL_UP },            /* 抬头：单步朝 -45° */
  { 0x11u, CMD_GIMBAL_DOWN },          /* 低头：单步朝 0° */
  { 0x14u, CMD_GIMBAL_PITCH_CENTER },  /* 俯仰回中 */
  { 0x16u, CMD_GIMBAL_YAW_SHAKE },     /* 摇头动画 */
  { 0x17u, CMD_GIMBAL_PITCH_NOD },     /* 点头动画：低头→抬头→回中 */
  { 0x12u, CMD_GIMBAL_LEFT },
  { 0x13u, CMD_GIMBAL_RIGHT },
  { 0x15u, CMD_GIMBAL_YAW_STOP },
  /* 底盘（后做） */
  { 0x01u, CMD_STOP },
  { 0x02u, CMD_FWD },
  { 0x03u, CMD_BACK },
  { 0x04u, CMD_LEFT },
  { 0x05u, CMD_RIGHT },
  /* 表情包 V1.0：ID → 静态脸（同 ID 的两种说法共用一帧） */
  { 0x20u, CMD_FACE_HELLO },
  { 0x21u, CMD_FACE_HAPPY },
  { 0x22u, CMD_FACE_CUTE },
  { 0x23u, CMD_FACE_ANGRY },
  { 0x24u, CMD_FACE_SLEEP },
  { 0x25u, CMD_FACE_LISTEN },
  { 0x26u, CMD_FACE_CONFUSED },
  { 0x27u, CMD_FACE_SURPRISE },
  { 0x28u, CMD_FACE_SAD },
  { 0x29u, CMD_FACE_WINK },
  { 0x2Au, CMD_FACE_REJECT },
};

static inline int vc_voice_id_to_cmd(uint8_t asr_id, vc_cmd_t *cmd_out)
{
  unsigned i;
  if (!cmd_out) return 0;
  for (i = 0; i < (sizeof(VC_VOICE_WORD_MAP) / sizeof(VC_VOICE_WORD_MAP[0])); ++i) {
    if (VC_VOICE_WORD_MAP[i].asr_id == asr_id) {
      *cmd_out = VC_VOICE_WORD_MAP[i].cmd;
      return 1;
    }
  }
  return 0;
}

#endif
