#pragma once

#include "cmd_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Return 1 if line was consumed (do not forward to STM32 as text). */
typedef int (*link_pc_line_hook_t)(const char *line);

void link_stm32_init(void);
/** 运动命令写 6 字节帧；表情命令写 bringup 文本 FACE n。返回 1 表示 UART 写完。 */
int link_stm32_send(vc_cmd_t cmd);
/** Optional: PC CH343 文本行钩子（用于 VC 命令注入，避免 ASR 未就绪时无法验收舵机）。 */
void link_stm32_set_pc_line_hook(link_pc_line_hook_t hook);
/** Mirror a line to CH343 (COM9). Console ESP_LOG 走 USB-JTAG，测语音请用这个。 */
void link_stm32_pc_write(const char *s);

#ifdef __cplusplus
}
#endif
