#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "../../common/cmd_protocol.h"

void protocol_init(void);
/** Return 1 if byte completed a valid frame; fills cmd/seq on success. */
int protocol_feed(uint8_t byte, vc_cmd_t *cmd_out, uint8_t *seq_out);
uint32_t protocol_accepted(void);
uint32_t protocol_rejected(void);

#endif
