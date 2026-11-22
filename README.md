# Voice Car — 云端 V0.3 设计 × 本地硬件

| 层 | 以谁为准 |
|---|---|
| **软件设计 / 协议 / 路由 / OLED 语义** | GitHub [`tortletortle/voice-car`](https://github.com/tortletortle/voice-car) V0.3 |
| **硬件引脚 / 波特率 / 实测 bringup** | 本仓库本地约定（`board.h` + `docs/wiring/`） |

双 MCU：ESP32-S3 汇聚语音/遥控，STM32F103 执行电机与云台。底板 PCB 在 `01_Projects/PCB`。

| 项 | 取值（硬件=本地） |
|---|---|
| 汇聚端 | ESP32-S3 DevKit（U2），板间 GPIO5 TX / GPIO4 RX，语音 GPIO17 RX / GPIO18 TX |
| 执行端 | STM32F103C8T6（U1），USART3 **PB11 RX / PB10 TX**，115200 |
| OLED | PB6 SCL / PB7 SDA，地址 `0x3C`→`0x3D`（bringup 已点亮） |
| 驱动 | MotA/MotB + STBY；**GPIO 未对脚前 fail-closed** |
| 云台 | SERVO1 俯仰：`0°=低头`，`-45°=抬头`；SERVO2 偏航（左右） |
| 帧协议 | `AA 55 CMD LEN SEQ CHK`（`firmware/common/`） |

---

## 1. 合并口径（必须）

1. **设计跟云端**：STOP 优先路由、滑动帧同步解析、`CMD_GIMBAL_YAW_STOP`、偏航显示 `STOP/LEFT/RGHT`、OLED 四行状态、USART3 512B 环缓意图。
2. **硬件跟本地**：不改已确认交叉脚、HSI bringup 波特、MotA/MotB 通道策略、PWM≤30%、舵机默认关（`VC_SERVO_OUTPUT_ENABLE=0`）。
3. **引脚只改 `board.h` / 组件顶部宏**，不散落进业务 `.c`。
4. 云端仓目前只有 common 切片；**以本目录为完整工作树**，再回推 GitHub。

---

## 2. 架构

```text
CI1302 ──UART──► ESP32-S3 ──UART 帧──► STM32F103 ──► MotA/MotB (fail-closed until map)
                     │                      │
               cmd_router_core           protocol(frame_parser)
               STOP 优先+重试            USART3 RX ring → dispatch
                     │                      │
               voice_word_map(提案)      OLED UI 模型 (PB6/PB7)
                                        gimbal: pitch° / yaw pulse
```

---

## 3. 目录

```text
firmware/
  common/     ← 云端合同：protocol / frame_parser / router_core / voice_word_map
  esp32/      ← 本地 GPIO + 云端路由核心
  stm32/      ← 本地板级 + V0.3 状态机 / OLED 模型 / bringup
docs/
  OLED_UI_V0.3.md
  INTEGRATION_GAPS.md
  wiring/     ← 本地接线权威
```

---

## 4. 命令表（V0.3）

| 值 | 符号 | 作用 |
|---|---|---|
| `0x01` | `CMD_STOP` | 立刻停；路由最高优先 |
| `0x02`–`0x05` | FWD/BACK/LEFT/RIGHT | 限时差速（1500/800 ms，PWM≤30%） |
| `0x10`/`0x11` | GIMBAL_UP/DOWN | 俯仰步进，限幅 45～135 |
| `0x12`/`0x13` | GIMBAL_LEFT/RIGHT | SERVO2 **定住** 左1000µs / 右2000µs |
| `0x14` | GIMBAL_PITCH_CENTER | **仅俯仰回中**（旧 CENTER 同义） |
| `0x15` | GIMBAL_YAW_STOP | SERVO2 **回中** 1500µs |
| `0x16` | GIMBAL_YAW_SHAKE | 摇头：左→右→左→回中 |

ESP 仲裁：`vc_router_state_*` — STOP 清掉排队普通命令；发送失败则重试 STOP。

---

## 5. 构建

### ESP32-S3

```bat
cd firmware\esp32
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

### STM32

- 正式逻辑：Keil 加 `App`/`Mid`/`Bsp` + `../common`（含 `cmd_frame_parser.c`），SysTick 1 ms。
- bringup：`firmware/stm32/bringup/bringup.hex`（链路 + OLED OK + SERVO OFF）。

---

## 6. 验收（不虚构实测）

| 阶段 | 看什么 |
|---|---|
| Phase 0 | bringup：`LINK` / `OLED OK`；ESP 见 `stm32: LINK` |
| Phase 1 | 帧解析 allow-list + 校验；STOP 优先；运动限时自停；电机未对脚时无驱动力 |
| Phase 2 | OLED 四行与表情；词表 ID 与模块固件一致后 `voice_uart`→`submit` |

---

## 7. 禁止

1. 把云端 partial 仓当成已可烧录整车  
2. 未对 PCB 网表就开电机/舵机 PWM  
3. 偏航屏显假角度  
4. 业务文件硬编码引脚  
5. 共 5V 联调盲目抬 PWM  

详见 `docs/INTEGRATION_GAPS.md`、`SYNC_STATUS.md`。
