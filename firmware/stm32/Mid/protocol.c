#include "protocol.h"
#include "../../common/cmd_frame_parser.h"

static vc_frame_parser_t s_parser;

void protocol_init(void)
{
  vc_frame_parser_init(&s_parser);
}

int protocol_feed(uint8_t byte, vc_cmd_t *cmd_out, uint8_t *seq_out)
{
  uint8_t seq = 0;
  vc_cmd_t cmd = CMD_NOP;
  if (!vc_frame_parser_feed(&s_parser, byte, &cmd, &seq)) {
    return 0;
  }
  if (cmd_out) *cmd_out = cmd;
  if (seq_out) *seq_out = seq;
  return 1;
}

uint32_t protocol_accepted(void)
{
  return s_parser.accepted;
}

uint32_t protocol_rejected(void)
{
  return s_parser.rejected;
}
