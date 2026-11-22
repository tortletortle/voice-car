# Voice Car — 语音控制小车固件

双 MCU 架构：ESP32-S3 做语音/遥控汇聚，STM32F103 做电机与云台执行。底板 PCB 在独立工程维护，本仓库只管固件与协议。

| 项 | 取值 |
|---|---|
| 汇聚端 | ESP32-S3 DevKit（U2），ESP-IDF |
| 执行端 | STM32F103C8T6（U1），逻辑骨架 + bare-metal bringup |
| 语音 | CI1302（UART，词表映射待补） |
| 驱动 | TB6612 风格双桥，仅 MotA/MotB |
| 板间链路 | UART 115200，帧协议见 `firmware/common/cmd_protocol.h` |
| OLED | 0.96″ I2C（硬件有座；文案规范已定，正式 UI 未合入主逻辑） |
| 相邻工程 | 底板 `01_Projects/PCB`；旧概念稿 `01_Projects/voice-car-pcb`（勿当基线） |

---

## 1. 系统架构

```text
CI1302 ──UART──► ESP32-S3 ──UART 帧──► STM32F103 ──► 电机 (MotA/MotB)
                     │                      │
               联网遥控(占位)              云台舵机 SERVO1/2
                     │                      │
               cmd_router 仲裁            OLED(Phase2 / bringup 实测)
```

| 角色 | 职责 | 现状 |
|---|---|---|
| ESP32 `voice_uart` | 收语音模块字节流 | 已接 GPIO17/18（VOICE_RX/TX）；先探 115200/9600 |
| ESP32 `remote_net` | WiFi/TCP 等遥控 | **占位 stub** |
| ESP32 `cmd_router` | 队列长度 1，最新命令覆盖 | 已实现 |
| ESP32 `link_stm32` | 打包 `AA 55 …` 下发；收 STM32 文本行日志 | GPIO4/5 + UART2 已接 |
| STM32 Mid | `protocol` 解帧、`motion` 限时自停、`gimbal` 限幅步进 | 逻辑已写 |
| STM32 Bsp | `uart_link` / `motor` / `servo` | **多为 TODO 桩**（待 Keil/HAL 接线） |
| STM32 `bringup/` | 无 RTOS 最小实测：USART3 + OLED「OK」+ 舵机停 | 可烧 `bringup.hex` |

数据流一句话：**语音或遥控 → 归一成 `vc_cmd_t` → 最新优先 → 6 字节帧给 STM32 → 差速限时停 / 云台限幅。**

---

## 2. 目录结构

```text
voice_car/
├── README.md                 ← 本文档
├── docs/
│   └── oled-content.md       ← OLED 四行文案规范（与协议对齐）
└── firmware/
    ├── common/
    │   └── cmd_protocol.h    ← 两端共享命令与帧格式
    ├── esp32/                ← ESP-IDF 工程
    │   ├── main/app_main.c
    │   ├── components/
    │   │   ├── voice_uart/
    │   │   ├── remote_net/
    │   │   ├── cmd_router/
    │   │   └── link_stm32/
    │   ├── sdkconfig.defaults
    │   └── README.md
    └── stm32/
        ├── App/              ← main + 状态分发
        ├── Mid/              ← protocol / motion / gimbal
        ├── Bsp/              ← board.h + uart/motor/servo
        ├── bringup/          ← HSI + USART3 + OLED 点亮实测
        └── README.md
```

引脚与运动策略只改 `firmware/stm32/Bsp/board.h`（及 ESP 侧各组件顶部 GPIO 宏），不要散落改业务逻辑。

---

## 3. 板级约定（已冻结口径）

来源：`board.h` 注释（2026-11-15 用户确认）+ bringup 实测脚。

### 3.1 板间 UART

| 网名 | 含义 | ESP32-S3 | STM32（bringup） |
|---|---|---|---|
| `ESP32_TX` | 出 ESP32 → 进 STM32 RX | GPIO5（TX） | PB11（USART3_RX，U1.36） |
| `ESP32_RX` | 出 STM32 → 进 ESP32 RX | GPIO4（RX） | PB10（USART3_TX，U1.35） |

- 波特率：**115200 8N1**
- bringup 用 **HSI 8 MHz**，`BRR=0x45`，实际约 115942（误差 ~0.6%）
- 连接器脚号 ≠ 随意猜 PA9；以最小系统卡丝印为准

### 3.2 语音 UART（CI1302 ↔ ESP32）

| 信号 | ESP32 GPIO |
|---|---|
| VOICE_RX（模块→ESP） | GPIO17 |
| VOICE_TX（ESP→模块） | GPIO18 |

上电后先听 115200，无字节再试 9600；词表→`vc_cmd_t` 映射仍待补。

### 3.3 电机

- 只用 **A/B** 通道（MotA 左、MotB 右）；C/D 本版不驱动
- 共用 **STBY**：有运动才拉高，停止拉低
- 默认前进/后退 **1500 ms** 自停；转向 **800 ms**
- 默认 PWM **≤30%**（电机与舵机共 5V，联调硬帽）
- 实装左右反了改 `MOTOR_SWAP_LR`；单轮方向反了改 `MOTOR_INVERT_L/R`

### 3.4 云台

| 项 | 值 |
|---|---|
| SERVO1 (PA0) | 俯仰：`0°=低头`，`-45°=抬头`，行程 [-45,0]，步进 5° |
| SERVO2 (PA1) | 偏航（左右） |
| CENTER | 俯仰中点 `-22°` |

### 3.5 OLED

| 项 | 值 |
|---|---|
| 接口 | I2C，PB6=SCL / PB7=SDA（U1.14 / U1.15） |
| 地址 | 先试 `0x3C`，失败再 `0x3D` |
| 正式 UI | `OLED_ENABLED=0`（Phase 2）；文案见 `docs/oled-content.md` |
| bringup | 成功则显示放大「OK」，串口打 `OLED OK` |

---

## 4. 命令协议（v0）

文件：`firmware/common/cmd_protocol.h`

### 4.1 帧格式（6 字节）

```text
AA 55  CMD  LEN  SEQ  CHK
         │    │    │    └─ CMD ^ LEN ^ SEQ
         │    │    └────── 序号（ESP 侧自增）
         │    └─────────── v0 固定为 1
         └──────────────── vc_cmd_t
```

### 4.2 命令表

| 值 | 符号 | 作用 |
|---|---|---|
| `0x00` | `CMD_NOP` | 无操作 |
| `0x01` | `CMD_STOP` | 立刻停，STBY 拉低 |
| `0x02` | `CMD_FWD` | 双轮前进，限时 |
| `0x03` | `CMD_BACK` | 双轮后退，限时 |
| `0x04` | `CMD_LEFT` | 原地左转，限时较短 |
| `0x05` | `CMD_RIGHT` | 原地右转，限时较短 |
| `0x10` | `CMD_GIMBAL_UP` | 俯仰 +步进 |
| `0x11` | `CMD_GIMBAL_DOWN` | 俯仰 −步进 |
| `0x12` | `CMD_GIMBAL_LEFT` | 偏航 +步进 |
| `0x13` | `CMD_GIMBAL_RIGHT` | 偏航 −步进 |
| `0x14` | `CMD_GIMBAL_CENTER` | 回中 |

仲裁策略：语音与遥控都 `cmd_router_submit`；队列长 1，**后到覆盖先到**。

---

## 5. 构建与烧录

### 5.1 ESP32-S3

```bat
cd firmware\esp32
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

- `sdkconfig.defaults`：USB Serial/JTAG 控制台；Flash 16MB
- 烧前核对 GPIO4/5（板间）与 GPIO17/18（语音 VOICE_RX/TX）是否与当前板一致

### 5.2 STM32 正式逻辑（骨架）

1. Keil MDK 新建工程，Device 选 **STM32F103C8**（或卡丝印型号）
2. 加入 `App` / `Mid` / `Bsp` 与 `../common`
3. 补 SPL/HAL、启动文件、时钟；**SysTick = 1 ms**
4. 按 PCB 填实 `uart_link` / `motor` / `servo` 的 TODO
5. 左右极性用 `board.h` 宏校正，勿改 `motion.c` 语义

### 5.3 STM32 bringup（当前可实测）

产物：`firmware/stm32/bringup/bringup.hex`（另有 `.elf` / `.bin`）

预期串口（USART3 ↔ ESP 链路脚，115200）：

```text
OLED OK   或  OLED NO
SERVO OFF
LINK          （约每 500 ms）
```

OLED 亮则屏显「OK」；PC13 板载 LED 常亮。

---

## 6. 验收清单（按阶段）

### Phase 0 — 链路与显示（bringup）

- [ ] STM32 烧 bringup 后串口周期性 `LINK`
- [ ] OLED 显示 OK 或明确 `OLED NO`（地址/接线问题）
- [ ] ESP32 `link_stm32` 能打出 `stm32: LINK` 一类日志

### Phase 1 — 命令闭环

- [ ] ESP 侧能发出完整 6 字节帧；STM32 `protocol_feed` 解出正确 `vc_cmd_t`
- [ ] `CMD_FWD` 等运动限时后自停；`CMD_STOP` 立即停
- [ ] 云台步进不超 45～135；回中到 90/90
- [ ] PWM 联调不超过 30%

### Phase 2 — 语音与 UI

- [ ] CI1302 词表 → `vc_cmd_t` 映射表落地
- [ ] OLED 按 `docs/oled-content.md` 四行状态屏刷新
- [ ] `remote_net` 可提交与语音同等命令

---

## 7. 已知缺口 / 禁止项

| 缺口 | 说明 |
|---|---|
| STM32 Bsp 桩 | `uart_link` / `motor` / `servo` 外设寄存器未写满 |
| 语音词表 | `voice_uart` 只收字节日志，不 `submit` 命令 |
| 联网遥控 | `remote_net` 仅日志 stub |
| OLED 主逻辑 | `OLED_ENABLED=0`；正式驱动未合入 App |
| Keil 工程文件 | 仓库未带 `.uvprojx`，需本机新建 |

**禁止：**

1. 把 Proteus / 仅编译通过当成实机通过  
2. 共 5V 联调时盲目拉高 PWM（默认帽 30%）  
3. 把 `voice-car-pcb` 旧概念稿当现行电气基线（以 `01_Projects/PCB` / EDA 回读为准）  
4. 在业务 `.c` 里硬编码引脚（改 `board.h` / 组件顶部宏）  
5. bringup 未确认舵机驱动前强行开 TIM2 PWM

---

## 8. 相关文档

| 文档 | 内容 |
|---|---|
| [docs/oled-content.md](docs/oled-content.md) | OLED 四行布局、运动态/命令标签字典、样例帧 |
| [firmware/esp32/README.md](firmware/esp32/README.md) | ESP 组件与构建摘要 |
| [firmware/stm32/README.md](firmware/stm32/README.md) | STM32 目录与 Keil 接入 |
| `01_Projects/PCB` | 智能小车底板 PCB 接手区 |

---

*文档生成自仓库现状（2026-11-19）。实测结论仅记录 bringup/代码已写明的部分，不虚构示波器或联调数据。*
