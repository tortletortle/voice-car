# Voice Car — V0.3 OLED status and expression integration

V0.3 builds on the V0.2 dual-MCU control foundation. It integrates a 128×64 OLED status display and an interrupt-buffered STM32 USART3 receive path. This is still a source/compile milestone, not a physical vehicle validation release.

## V0.3 changes

1. Added the OLED software-I²C driver on the source-confirmed PB6/PB7 pins. It probes address `0x3C`, then `0x3D`.
2. Added a four-line, 16-character status screen and a small pixel-art expression face. The screen updates only when displayed state changes and flushes at most one 128-byte page per main-loop pass.
3. Corrected the yaw display semantics: the 360° continuous-rotation servo has no position feedback, so the screen shows `STOP`, `LEFT`, or `RGHT`, not a fabricated angle. Pitch is displayed as the commanded setpoint, not measured feedback.
4. Added a 512-byte USART3 RX ring buffer populated by the USART3 interrupt. This prevents the OLED page write from being the only place UART bytes can be received.
5. Expanded the Cortex-M3 vector table and connected USART3 IRQ 39 to `USART3_IRQHandler`.
6. Added a portable OLED UI model and host tests for status formatting, command labels, expression selection, and motion-state retention after timed stop.

## OLED layout

```text
VC  IDLE              [face]
CMD ----  S--
P090 Y STOP
M:OFF S:OFF R:--
```

- `P090` is the commanded pitch position.
- `Y STOP / LEFT / RGHT` is the continuous-yaw command state, not a measured heading.
- `M:OFF` means the board-specific motor backend is not configured.
- `S:OFF` means servo PWM is disabled by default.
- `R:OK` means at least one valid command frame has been received since boot; it is not a heartbeat or live-link guarantee.

See `docs/OLED_UI_V0.3.md` for details.

## Source layout

- `firmware/common/`: shared command contract, parser, and ESP32 router core.
- `firmware/esp32/`: ESP-IDF app, UART2 board link, UART1 voice parser and ID map.
- `firmware/stm32/`: USART3 RX interrupt/ring buffer, command dispatch, motion/gimbal state machines, OLED driver and guarded actuator backends.
- `tests/`: host unit tests, hardware mocks, and ESP-IDF API stubs.
- `docs/INTEGRATION_GAPS.md`: physical facts still missing and not guessed.

## Verification commands

```bash
./tests/run_host_tests.sh
./scripts/check_esp32_syntax.sh
./scripts/build_stm32_safe.sh
```

The STM32 build script uses Clang's `arm-none-eabi` target and `llvm-objcopy`. It produces `build/arm/voice_car_stm32_safe.elf`, `.hex`, and `.bin`.

## Safety and evidence limits

- `motor_port_unconfigured.c` remains fail-closed. The supplied materials do not map TB6612 A/B inputs, PWM and STBY to STM32 GPIOs. Movement commands will not energize the motors.
- `VC_SERVO_OUTPUT_ENABLE` defaults to `0`. The physical SERVO1/SERVO2 connector mapping is not proven. PWM is compile-time gated until the current PCB netlist confirms it.
- Yaw pulse values are calibration starting points, not guaranteed values for every continuous servo.
- `voice_word_map.h` IDs are proposals that must match a custom Module ASR firmware build. The physical module has not been configured or tested here.
- ESP32 receives no structured STM32 ACK/telemetry yet. Its local UART write-complete result is not proof that STM32 executed a command.
- The OLED address/controller and actual display refresh have not been tested on the physical board. `OLED_COLUMN_OFFSET` defaults to 0; set it to 2 if a confirmed SH1106 panel's visible area is shifted.
- Phone remote control and AI control are not integrated because no working `remote_net` implementation was present in the supplied archive.

Automated tests and cross-compilation do not establish physical wiring, board flashing, electrical compatibility, or actual vehicle behavior.
