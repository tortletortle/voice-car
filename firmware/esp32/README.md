# ESP32-S3 遥控汇聚端（ESP-IDF）

职责：CI1302 语音 UART + 联网遥控 → `cmd_router_core`（STOP 优先）→ UART 发给 STM32。

**设计**：云端 V0.3 路由核心  
**硬件**：本地 GPIO — 板间 TX=5 / RX=4；语音 TX=11 / RX=10

## 组件

| 组件 | 职责 |
|---|---|
| `voice_uart` | 收语音字节（词表→`voice_word_map.h` 后 `submit`） |
| `remote_net` | SoftAP `realme` + 网页遥控 `http://192.168.4.1/` |
| `cmd_router` | 包装 `cmd_router_core`；STOP 发送失败重试 |
| `link_stm32` | `vc_pack` 下发；返回本地写完成 |

## 构建

```bat
cd firmware\esp32
idf.py set-target esp32s3
idf.py build
```
