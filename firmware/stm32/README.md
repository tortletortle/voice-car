# STM32F103 执行端

职责：收 ESP32 命令帧 → 差速限时自停 → SERVO1 俯仰(0低头/-45抬头) + SERVO2 偏航定位(1000/1500/2000 + 摇头) → OLED 状态模型。

**设计**：GitHub V0.3（frame_parser / yaw-stop / OLED 语义）  
**硬件**：本地 `Bsp/board.h`（USART3 PB10/PB11、OLED PB6/PB7、MotA/MotB）

## 目录

| 路径 | 模块 |
|---|---|
| `App/` | 主循环：环缓→parser→dispatch→OLED 模型 |
| `Mid/` | `protocol` `motion` `gimbal` `oled_ui` |
| `Bsp/` | `uart_link` 环缓 / `motor` fail-closed / `servo` 门控 / `board.h` |
| `bringup/` | 已实测：LINK + OLED OK + SERVO OFF |
| `../common/` | `cmd_protocol` + `cmd_frame_parser` |

## Keil 接入

1. Device：STM32F103C8（或丝印型号）
2. 加入 `App` `Mid` `Bsp` 与 `../common`（含 `.c`）
3. SysTick = 1 ms；USART3 IRQ39 → `uart_link_rx_push`
4. 电机脚对 PCB 后：`MOTOR_GPIO_CONFIGURED=1`；舵机确认后：`VC_SERVO_OUTPUT_ENABLE=1`
