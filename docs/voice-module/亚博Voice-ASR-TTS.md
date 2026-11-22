# 亚博 Voice-ASR-TTS（本车应对模块）

**实物口径（用户链接确认）**  
- 教程：[AI语音交互集成模块](https://www.yahboom.com/study/Voice-ASR-TTS)  
- 课程：[build id=13114](https://www.yahboom.com/build?id=13114&cid=704)  

**不是** M5 Module ASR。芯片仍是 CI1302，但板载 **STC8H** 转串口/IIC，出厂词表与帧格式按**亚博固件**，不能用 `Hi, M Five` / `AA 55 ID 55 AA` 那套硬套。

---

## 1. 硬件要点

| 项 | 说明 |
|---|---|
| 芯片 | CI1302 + STC8H 协处理 |
| 供电 | **DC 5V**（红灯常亮=供电正常）；超 5V 会损坏 |
| Type-C | 供电 + 烧录下载 |
| 喇叭/麦 | 板载；识别后会**播报** |
| 滑动开关 | 切换烧录串口 / 正常工作（拨错则无正常识别或无法烧录） |
| RST | 复位；烧录时常配合按下 |
| 串口/IIC | 对外通信；串口可控制被动播报 |

参考硬件说明：[博客园硬件接口表](https://www.cnblogs.com/FBsharl/p/18974491)、同类资料 [钜犀产品介绍](https://wiki.juxitech.com/zh-hans/tutorials/accessories/ai-voice-module/Product-Info)。

---

## 2. 发声验收（先过这一关）

### 2.1 上电

1. Type-C 插电脑（或 H1 +5V/GND）。  
2. **红灯应常亮**。不亮 → 供电问题，先别谈协议。  
3. 滑动开关拨到**正常工作**档（非烧录档；以教程图为准）。

### 2.2 唤醒（出厂常见，按你固件二选一试）

| 说法 | 期望喇叭 |
|---|---|
| **`Hi, Yahboom`**（慢一点） | **`I am here` / `I'm here`** |
| **`小亚小亚`** | **`我在`** / I'm here 类 |
| （部分 OEM）`你好，小犀` | 同类应答 |

官方英文 quick start：[Get started quickly](https://www.yahboom.net/public/upload/upload-html/1742021302/0.%20Get%20started%20quickly.html) 写唤醒词 **`Hi, Yahboom`**，成功回 **`I am here`**；约 20s 无命令会休眠并可能播休息语。

**禁止再用 `Hi, M Five` 当本模块默认唤醒。**

### 2.3 命令例（唤醒后）

英文车控类（见 [Introduction](https://www.yahboom.net/public/upload/upload-html/1762853904/1.%20Introduction%20of%20voice%20module.html)）：

| 说法 | 模块→主机帧（示例） | 播报例 |
|---|---|---|
| Car Forward / Go ahead | `AA 55 00 04 FB` | OK, Moving Forward |
| Car Stop | `AA 55 00 01 FB` | OK, Stopped |

---

## 3. 串口协议（与 M5 不同）

亚博出厂常见帧（5 字节）：

```text
AA 55 <mode> <cmd> FB
```

| mode | 含义 |
|---|---|
| `00` | 主动播报相关（识别上报常见） |
| `FF` / `01` 等 | 被动播报 / 其它（以协议表为准） |

唤醒例：`AA 55 03 00 FB`（见 [Voice interaction control](https://www.yahboom.net/public/upload/upload-html/1763370976/Voice%20interaction%20control.html) 表中 HI-YAHBOOM）。

**本仓 `voice_uart` 当前按 M5 `AA 55 ID 55 AA` 解析 → 对亚博帧会解错/解不出。**  
发声通过后，再改状态机与 `voice_word_map`。

波特率：资料多用 **115200 8N1**。

---

## 4. 与底板接线（不变）

H1：`+5V / GND / VOICE_RX / VOICE_TX` → ESP GPIO17/18（交叉）。  
模块 TX → `VOICE_RX`；模块 RX ← `VOICE_TX`。

---

## 5. 失败树（亚博版）

```text
无声
 ├─ 红灯不亮？ → 5V 供电
 ├─ 滑动开关在烧录档？ → 拨回运行档，按 RST
 ├─ 说的是 Hi M Five？ → 改说 Hi Yahboom / 小亚小亚
 └─ 红灯亮、词对仍无声？ → 查亚博教程重烧出厂固件（拨码+RST）
```

完整资料包：以 [yahboom.com/study/Voice-ASR-TTS](https://www.yahboom.com/study/Voice-ASR-TTS) 下载区协议表 Excel 为准。
