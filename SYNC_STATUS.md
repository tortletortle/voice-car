# Sync status — cloud design × local hardware

## Policy

- **Design source of truth:** GitHub `tortletortle/voice-car` V0.3 (protocol, parser, router, OLED semantics, continuous yaw).
- **Hardware source of truth:** local `firmware/stm32/Bsp/board.h` + `docs/wiring/` + bringup (USART3 PB10/PB11, OLED PB6/PB7, ESP GPIO4/5/10/11, MotA/MotB policy).

## Remote vs local (2026-11-21)

| Location | Content |
|---|---|
| GitHub remote | Partial: README + CHANGELOG + `firmware/common/` only (8 files) |
| This workspace | Full working tree: ESP32 + STM32 + docs + bringup, now aligned to V0.3 design |

## Imported / implemented locally

- [x] `cmd_protocol.h` (+ `CMD_GIMBAL_YAW_STOP`, allow-list)
- [x] `cmd_frame_parser.*`
- [x] `cmd_router_core.*` wired into ESP `cmd_router`
- [x] Continuous yaw gimbal + pitch-center-only
- [x] USART3 RX ring API (512 B); IRQ wiring still TODO
- [x] OLED UI model (four lines, yaw = STOP/LEFT/RGHT)
- [x] Motor fail-closed until `MOTOR_GPIO_CONFIGURED=1`
- [x] `VC_SERVO_OUTPUT_ENABLE` default 1（2026-11-22 打开；App TIM2 已接线，待烧 App 验收）
- [x] OLED expression pack V1.0 已落本地：`assets/oled-expression-pack-v1.0/`（来自 GitHub `feature/oled-expression-pack-v1.0`）
- [x] 表情轮播调试器：`tools/oled_face_carousel.py`；bringup 已含 `FACE/NEXT/PREV/AUTO/ANIM`
- [x] 新 `firmware/stm32/bringup/bringup.hex` 已用 xpack ARM GCC 编出（~6.2KB text；旧包 `bringup.hex.bak_pre_face`）
- [x] 2026-11-22 OpenOCD+ST-Link 烧录 verify OK；脚本 `bringup/flash.ps1`；串口验收待你关占口后看 `READY S1 S2 FACE`
- [ ] Physical OLED page flush driver in App（正式四行字 + 表情合成仍待）
- [ ] USART3 IRQ39 → `uart_link_rx_push`
- [ ] TB6612 GPIO map from PCB
- [x] Gimbal voice cmds: UP/DOWN one-shot + YAW_SHAKE + **PITCH_NOD 0x17**（低头→抬头→回中）
- [x] ESP PC inject `VC NOD/SHAKE/UP/DOWN`（`tools/gimbal_vc_test.py`）绕过 ASR 先看舵机
- [x] `remote_net` SoftAP `realme` + HTTP 网页遥控 `http://192.168.4.1/`（2026-11-22 已编译通过；待烧录实机验）
- [ ] Voice ASR ID map verified on hardware
- [x] Push this tree back to GitHub as complete baseline（2026-11-22；协议 ID 仍以本地为准，见 CHANGELOG）

## 舵机（2026-11-22：**暂停，未做完**）

用户口径：先停，**不要当成交付完成**。

已留下的现场资产（可续）：

- bringup 双路手调：`S1/S2 <0..19999>` / `OFF` / `ALL OFF`；GUI：`tools/servo_tune_gui.py`
- ESP32 COM9 桥：STM32 ↔ 电脑文本命令
- SERVO1 俯仰脉宽手调记入 `board.h`：抬头 **-45°=500µs**，低头 **0°=1000µs**，中点约 750µs

未完（续做时接着）：

- App `servo.c` 度数→脉宽 / TIM PWM 仍是 TODO；`VC_SERVO_OUTPUT_ENABLE` 仍默认 0
- SERVO2 定位：左1000 / 中1500 / 右2000；`0x16` 摇头；App `servo.c` 已接 TIM2；bringup COM9 已跑 `servo2_yaw_test.py`（看舵机现象需你确认）
- 正式联调开舵机 PWM 前仍须对 PCB / 共 5V 帽

## Do not

Treat the remote-only snapshot as a buildable vehicle release. Preserve local bringup-proven pins when merging further cloud code.
