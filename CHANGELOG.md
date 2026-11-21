# Changelog

## Unreleased — Voice command contract (2026-11-21)

- Preserved existing chassis command IDs and added stable shared IDs for nod, shake, repeated nod/shake, and neutral head action.
- Added Chinese voice phrase aliases for forward, backward, left, right, stop, head expressions, greeting, agreement, and refusal.
- Split the shared router into independent chassis and gimbal pending slots; STOP clears stale chassis motion, blocks new motion until sent, and retains queued head actions.
- Added host regression tests for command IDs, independent queues, latest-command behavior, STOP priority, and STOP retry.
- Scope: shared protocol/router and command specification only. CI1302 word IDs, target-specific ESP32/STM32 handlers, and physical servo behavior remain unverified in this partial remote snapshot.

## V0.3 — 2026-11-20

- Integrated the OLED status UI on source-confirmed PB6/PB7 with 0x3C → 0x3D address probing.
- Added a four-row 16-character status screen and a 16×16 pixel expression face.
- Corrected display semantics for the 360° continuous-rotation yaw servo: display command state (STOP/LEFT/RIGHT), not a fabricated angle. Pitch is explicitly a target setpoint, not measured feedback.
- Added a 512-byte USART3 RX ring buffer populated by the USART3 IRQ and expanded the vector table to route IRQ39 to `USART3_IRQHandler`.
- Added status snapshot plumbing from vehicle/motion/gimbal state to the OLED UI.
- Added host tests for OLED line formatting, status fields, expression selection, and retention of the last command after timed motion stop.
- Kept the motor backend fail-closed and servo PWM disabled; no unverified PCB pins were enabled.

## V0.2 — 2026-11-20

- Integrated the five-byte Module ASR parser and configurable suggested ID map from the earlier interface patch.
- Added a six-byte STM32 command frame parser with command allow-list, LEN/checksum validation and sliding resynchronization.
- Added a portable command router core and ESP-IDF critical-section wrapper. STOP clears stale normal commands, blocks normal commands while in flight, and is retried when local UART transmission does not complete.
- Changed yaw from false angle control to continuous-servo pulse-width control with local timeout; pitch remains a bounded positional servo.
- Added local timed chassis stop and a single main-loop integration point for UART RX and elapsed-ms ticking.
- Added a guarded TIM2/PA0/PA1 servo backend. Outputs remain disabled until the actual connector mapping is confirmed.
- Added a fail-closed motor backend because the supplied files do not identify the motor-driver GPIO pin map.
- Added host tests, ESP-IDF-facing syntax checks using API stubs, and a Cortex-M3 safe-output build.
