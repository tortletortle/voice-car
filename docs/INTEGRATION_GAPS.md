# Integration gaps

Facts still missing — **not guessed**. Design may be complete; hardware evidence is not.

| Gap | Status |
|---|---|
| TB6612 AIN/BIN/PW/STBY → STM32 GPIO | Unknown on current PCB netlist → `MOTOR_GPIO_CONFIGURED=0` |
| SERVO work **paused 2026-11-22 (not done)**; App TIM PWM still gated | `VC_SERVO_OUTPUT_ENABLE=0`; resume via bringup GUI / `board.h` pitch µs |
| SERVO1 pitch µs (hand-tuned) | 抬头 -45°=500µs；低头 0°=1000µs；中点≈750µs — App deg→µs TODO |
| SERVO2 positional yaw (1000/1500/2000) + SHAKE | Logic in `gimbal.c`; TIM PWM still gated / unproven on PCB |
| CI1302 custom ASR ID table | `voice_word_map.h` is proposal only |
| ESP←STM structured ACK | None; ESP TX-complete ≠ execution |
| OLED controller SSD1306 vs SH1106 | Bringup showed OK; `OLED_COLUMN_OFFSET` may need 2 |
| `remote_net` | SoftAP+HTTP 已实现（`realme`/`99999999`→`192.168.4.1`）；待烧录实机验 |
| USART3 IRQ → ring in Keil image | API ready; vector/IRQ not wired in skeleton |
| Push full tree to GitHub | Pending user request |

Local evidence that **does** stand:

- USART3 PB10/PB11 @ 115200 (bringup BRR=0x45 on HSI)
- OLED PB6/PB7 address probe path
- SERVO left off in bringup
