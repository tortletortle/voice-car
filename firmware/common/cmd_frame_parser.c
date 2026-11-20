#include "cmd_frame_parser.h"

static void drop_first(vc_frame_parser_t *p)
{
  uint8_t i;
  if (p->count == 0u) return;
  for (i = 1u; i < p->count; ++i) p->bytes[i - 1u] = p->bytes[i];
  --p->count;
}

void vc_frame_parser_init(vc_frame_parser_t *parser)
{
  if (parser == 0) return;
  parser->count = 0u;
  parser->accepted = 0u;
  parser->rejected = 0u;
}

void vc_frame_parser_reset(vc_frame_parser_t *parser)
{
  if (parser == 0) return;
  parser->count = 0u;
}

uint8_t vc_frame_parser_feed(vc_frame_parser_t *parser,
                             uint8_t byte,
                             vc_cmd_t *cmd_out,
                             uint8_t *seq_out)
{
  if ((parser == 0) || (cmd_out == 0) || (seq_out == 0)) return 0u;

  if (parser->count >= VC_FRAME_LEN) parser->count = 0u;
  parser->bytes[parser->count++] = byte;

  for (;;) {
    while (parser->count >= 1u && parser->bytes[0] != VC_FRAME_HEAD0) {
      drop_first(parser);
    }
    if (parser->count < 2u) return 0u;
    if (parser->bytes[1] != VC_FRAME_HEAD1) {
      drop_first(parser);
      continue;
    }
    if (parser->count < VC_FRAME_LEN) return 0u;

    if ((parser->bytes[3] == VC_FRAME_V0_LEN_FIELD) &&
        vc_cmd_is_known(parser->bytes[2]) &&
        (parser->bytes[5] == vc_checksum(parser->bytes[2],
                                         parser->bytes[3],
                                         parser->bytes[4]))) {
      *cmd_out = (vc_cmd_t)parser->bytes[2];
      *seq_out = parser->bytes[4];
      parser->accepted++;
      parser->count = 0u;
      return 1u;
    }

    parser->rejected++;
    /* Sliding resynchronization: a valid AA 55 may start inside bad bytes. */
    drop_first(parser);
  }
}
