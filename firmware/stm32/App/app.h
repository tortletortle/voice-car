#ifndef APP_H
#define APP_H

#include <stdint.h>

void app_init(void);
/** Call from main loop; pass ms since last call. */
void app_loop(uint32_t elapsed_ms);

#endif
