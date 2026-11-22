# ESP32-S3 遥控汇聚端（ESP-IDF）

职责：CI1302 语音 UART + 联网遥控 → 归一命令 → UART 发给 STM32。

## 组件

| 组件 | 职责 |
|---|---|
| `voice_uart` | 收语音模块（词表映射后补） |
| `remote_net` | 联网遥控占位 |
| `cmd_router` | 队列仲裁，最新命令优先 |
| `link_stm32` | 打包 `cmd_protocol.h` 帧下发 |

## 构建

```bat
cd firmware\esp32
idf.py set-target esp32s3
idf.py build
```

GPIO 占位见各组件源码顶部宏；对齐 PCB 的 VOICE / ESP32_TX·RX 后再烧。
