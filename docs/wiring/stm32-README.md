# STM32F103 执行端（Keil）

职责：收 ESP32 命令帧 → 差速限时自停 → 云台限幅。

## 目录

| 路径 | 模块 |
|---|---|
| `App/` | `main` + 状态分发 |
| `Mid/` | `protocol` / `motion` / `gimbal` |
| `Bsp/` | `uart_link` / `motor` / `servo` / `board.h` |
| `../common/cmd_protocol.h` | 与 ESP32 共享命令 |

## Keil 接入（待你本机确认工具链）

1. 新建 MDK 工程，Device 选 STM32F103C8（或你卡丝印型号）。
2. 把 `App` `Mid` `Bsp` 与 `../common` 加入工程。
3. 补 SPL/HAL 启动文件、时钟；SysTick 1 ms。
4. 按 PCB 改 `Bsp/board.h` 引脚；电机左右用 `MOTOR_SWAP_LR` / `MOTOR_INVERT_*`。

当前为可编译逻辑骨架，BSP 外设为 TODO 桩。
