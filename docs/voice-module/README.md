# Voice Car — 语音模块资料包（亚博 Voice-ASR-TTS / CI1302）

本包只收「语音模块侧」信息：接线、串口探测、协议参考、词条→车控映射、本仓相关源码摘录。

**本板模块（用户确认）**：亚博 [Voice-ASR-TTS](https://www.yahboom.com/study/Voice-ASR-TTS)（CI1302 + STC8H），**不是** M5 Module ASR。  
**本板实测**：ESP 侧 GPIO17/18（VOICE_RX/TX）；波特率优先 **115200 8N1**。

---

## 包内文件

| 文件 | 内容 |
|---|---|
| `README.md` | 本说明（入口） |
| `亚博Voice-ASR-TTS.md` | **SoT**：亚博硬件/唤醒/帧格式 |
| `发声验收.md` | **优先**：喇叭出声卡点、失败树 |
| `固件重烧备忘.md` | 无声时按亚博拨码+RST 重烧 |
| `接线与探测.md` | 模块↔ESP32 接线、波特率探测 |
| `协议参考.md` | 亚博帧（主）/ M5 / 启英泰伦 SDK |
| `词表映射草案.md` | 建议词条 → `vc_cmd_t` |
| `资料链接.md` | 亚博教程 + 其它对照入口 |
| `src/` | 本仓 ESP 语音 UART 摘录（仍为旧 M5 解析，待改） |

---

## 一句话架构

```text
CI1302(+STC8H) ──UART──► ESP32-S3 voice_uart ──映射──► cmd_router ──帧──► STM32
```

现状：`voice_uart` 仍按 M5 `AA 55 ID 55 AA` 解析 → **对亚博 `AA 55 00 XX FB` 会解错**；发声通过后改解析与 `voice_word_map`。  
旁路：PC 可发 `VC NOD` 等绕过 ASR。

---

## 本仓已冻结（相对可靠）

| 项 | 值 |
|---|---|
| 模块 | 亚博 Voice-ASR-TTS |
| 芯片 | CI1302 + STC8H |
| ESP 收模块 TX | **GPIO17**（`VOICE_RX`） |
| ESP 发模块 RX | **GPIO18**（`VOICE_TX`） |
| 外设 | UART1，**115200 8N1** |
| 车控命令目标 | `firmware/common/cmd_protocol.h` 的 `vc_cmd_t` |

---

## 使用顺序

1. **先读 `发声验收.md` / `亚博Voice-ASR-TTS.md`**：5V 红灯 → **`Hi, Yahboom`** 或 **`小亚小亚`** → 喇叭应答  
2. 无声：滑动开关/RST → 按 `固件重烧备忘.md`（亚博流程）  
3. 有声后再抓 `AA 55 … FB`，对照 `协议参考.md` 方案 **Y**  
4. 改 ESP 解析 + `词表映射草案.md`  
5. 自定义词表：走亚博教程「修改唤醒词和命令词」  

**禁止**：喇叭未出声时改词表/舵机；未确认本板 HEX 前把对照 ID 当量产映射。
