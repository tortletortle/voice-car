# Changelog

## Local merge — 2026-11-21

- Adopted GitHub V0.3 **software design** into `C51/voice_car`.
- Kept **local hardware**: USART3 PB10/PB11, OLED PB6/PB7, ESP GPIO4/5/10/11, MotA/MotB + PWM≤30% / timed stop.
- Imported `cmd_frame_parser` / `cmd_router_core` / extended `cmd_protocol`.
- Gimbal: continuous yaw pulse + `CMD_GIMBAL_YAW_STOP`; CENTER = pitch only.
- ESP router: STOP priority + retry on failed local UART write.
- Added OLED UI formatter (`oled_ui.*`) matching V0.3 layout; physical flush still TODO.
- Motor backend fail-closed via `MOTOR_GPIO_CONFIGURED=0`.

## V0.3 — 2026-11-20 (cloud design notes)

- OLED status UI on PB6/PB7 with 0x3C → 0x3D probing.
- Four-row 16-char screen + expression; yaw shows command state not angle.
- USART3 RX ring + IRQ39 intent.
- Host-testable UI model; motor/servo guarded.

## V0.2 — 2026-11-20 (cloud)

- Five-byte Module ASR parser + ID map proposal.
- Six-byte frame parser with allow-list / checksum / sliding resync.
- Portable router core; STOP clears normals and retries on TX fail.
- Continuous yaw pulse control; fail-closed motor without pin map.
