# Changelog

## Local tree push — 2026-11-22

- Pushed the full local working tree onto the private GitHub baseline.
- Kept the local wire IDs already used by ESP32/STM32: `0x16` = `CMD_GIMBAL_YAW_SHAKE`, `0x17` = `CMD_GIMBAL_PITCH_NOD`, face commands `0x20`–`0x2A`.
- Kept the remote-only files `docs/VOICE_COMMANDS_V1.md` and `tests/`. Those files still describe a different map (`0x16` = nod). They were not rewritten.

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

## Remote snapshot — Voice command contract (2026-11-21)

- Preserved existing chassis command IDs and added stable shared IDs for nod, shake, repeated nod/shake, and neutral head action.
- Added Chinese voice phrase aliases for forward, backward, left, right, stop, head expressions, greeting, agreement, and refusal.
- Split the shared router into independent chassis and gimbal pending slots; STOP clears stale chassis motion, blocks new motion until sent, and retains queued head actions.
- Added host regression tests for command IDs, independent queues, latest-command behavior, STOP priority, and STOP retry.
- Scope: shared protocol/router and command specification only. CI1302 word IDs, target-specific ESP32/STM32 handlers, and physical servo behavior remain unverified in this partial remote snapshot.

## Remote V0.3 — 2026-11-20

- Integrated the OLED status UI on source-confirmed PB6/PB7 with 0x3C → 0x3D address probing.
- Added a four-row 16-character status screen and a 16×16 pixel expression face.
- Corrected display semantics for the 360° continuous-rotation yaw servo: display command state (STOP/LEFT/RIGHT), not a fabricated angle. Pitch is explicitly a target setpoint, not measured feedback.
- Added a 512-byte USART3 RX ring buffer populated by the USART3 IRQ and expanded the vector table to route IRQ39 to `USART3_IRQHandler`.
- Added status snapshot plumbing from vehicle/motion/gimbal state to the OLED UI.
- Added host tests for OLED line formatting, status fields, expression selection, and retention of the last command after timed motion stop.
- Kept the motor backend fail-closed and servo PWM disabled; no unverified PCB pins were enabled.

## Remote V0.2 — 2026-11-20

- Integrated the five-byte Module ASR parser and configurable suggested ID map from the earlier interface patch.
- Added a six-byte STM32 command frame parser with command allow-list, LEN/checksum validation and sliding resynchronization.
- Added a portable command router core and ESP-IDF critical-section wrapper. STOP clears stale normal commands, blocks normal commands while in flight, and is retried when local UART transmission does not complete.
- Changed yaw from false angle control to continuous-servo pulse-width control with local timeout; pitch remains a bounded positional servo.
- Added local timed chassis stop and a single main-loop integration point for UART RX and elapsed-ms ticking.
- Added a guarded TIM2/PA0/PA1 servo backend. Outputs remain disabled until the actual connector mapping is confirmed.
- Added a fail-closed motor backend because the supplied files do not identify the motor-driver GPIO pin map.
- Added host tests, ESP-IDF-facing syntax checks using API stubs, and a Cortex-M3 safe-output build.
