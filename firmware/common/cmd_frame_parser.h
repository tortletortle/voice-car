#ifndef VOICE_CAR_CMD_FRAME_PARSER_H
#define VOICE_CAR_CMD_FRAME_PARSER_H

#include <stdint.h>
#include "cmd_protocol.h"

typedef struct {
  uint8_t bytes[VC_FRAME_LEN];
  uint8_t count;
  uint32_t accepted;
  uint32_t rejected;
} vc_frame_parser_t;

void vc_frame_parser_init(vc_frame_parser_t *parser);
void vc_frame_parser_reset(vc_frame_parser_t *parser);
/* Feed one byte. Returns 1 only for a complete, valid v0 command frame. */
uint8_t vc_frame_parser_feed(vc_frame_parser_t *parser,
                             uint8_t byte,
                             vc_cmd_t *cmd_out,
                             uint8_t *seq_out);

#endif
