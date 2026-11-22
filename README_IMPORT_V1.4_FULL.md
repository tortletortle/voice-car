# Voice Car V1.4.0 — STM32 ACK 回传手机导入说明

本版本基于完整 `integration/v1.3-android-voice-commands` 工程，增加 ESP32 request-token / STM32 ACK 关联、TCP JSONL 处理阶段与 capability 返回，以及 Android 端结构化状态显示。

## 交付文件

- `voice-car-v1.4-android-stm32-ack.git.bundle`：完整 Git 历史与 `integration/v1.4-android-stm32-ack` 分支。
- `voice-car-v1.4-android-stm32-ack.zip`：完整源码快照。
- `tools/import_voice_car_v1_4_to_github.py`：安全导入脚本。

## 导入 GitHub

在已配置 Git 与 GitHub 凭据的 WSL2 Ubuntu / Windows Git Bash / Linux 中，解压 ZIP，将 `.git.bundle` 放在项目根目录，然后执行：

```bash
python3 tools/import_voice_car_v1_4_to_github.py
```

也可以显式指定 bundle：

```bash
python3 tools/import_voice_car_v1_4_to_github.py --bundle /path/to/voice-car-v1.4-android-stm32-ack.git.bundle
```

脚本会校验完整 bundle 和关键文件，从最新远端 `main` 创建独立分支 `integration/v1.4-android-stm32-ack`，导入完整源码，普通 push 并 fetch 核对 SHA。不会修改或强推 `main`；目标分支已存在时会拒绝覆盖。

## 本轮实现

- STM32 ACK 在 ESP32 端按 UART `SEQ+CMD` 关联到对应手机 request token。
- ESP32 等待 ACK 最多 250ms；超时返回 `stage=esp32_queued`，匹配成功返回 `stage=stm32_app`、`seq`、`result`、`capabilities`。
- Android 显示 STM32 App 软件 ACK、仅 ESP32 入队、执行器 BSP 不可用等状态。
- ACK 不是电机/舵机物理反馈。

## 验证边界

`./tests/run_all_checks.sh` 已通过主机/Mock、ESP32 API stub 语法、STM32 syntax、sanitizer、GCC analyzer 和 Cortex-M3 safe-default/bench 链接。当前环境没有 Android SDK/Gradle，也没有 ESP-IDF 目标工具链；APK、目标板构建、真实 UART/TCP、手机安装与实物执行器动作尚未验收。
