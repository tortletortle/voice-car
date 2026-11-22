# Voice Car V1.4.0 — STM32 ACK 回传 Android 测试报告

日期：2026-11-22
基线：`integration/v1.3-android-voice-commands`
范围：ESP32 request-token/ACK 关联、TCP JSONL 状态响应、Android 结构化结果解析、完整主机回归。

## 1. 本轮实现

- ESP32 `link_stm32` 对手机请求保留 request ID、UART sequence 和 command ID 的关联记录。
- STM32 ACK 必须同时匹配 sequence 与 command 才能完成对应请求；不匹配、过期、心跳 ACK 不会被错误关联。
- ESP32 命令路由增加请求完成队列；TCP 请求等待 STM32 ACK 最多 250ms。
- TCP 响应新增 `stage`、`seq`、`result`、`capabilities`、`warning`。
- Android `RemoteClient` 返回 `RemoteActionResult`，状态栏区分 STM32 App ACK 与仅 ESP32 入队，并显示 motor/servo capability 未启用的情况。
- `result=2`（执行器 BSP 不可用）作为拒绝响应返回，不会显示成成功。

## 2. 自动化结果

| 测试 | 结果 | 证据范围 |
|---|---|---|
| `test_link_status_protocol` | PASS | ACK 帧打包、校验、流式解析、重同步及未知结果码拒绝 |
| `test_link_stm32_tracking` | PASS | 乱序 ACK、SEQ+CMD 匹配、错误 CMD、过期 ACK、NOP 心跳隔离；使用 UART/FreeRTOS stub |
| `test_cmd_router_integration` | PASS | request token 从路由事件传到链路并进入完成队列；mock ACK handler |
| `test_full_command_pipeline` | PASS | 六字节命令帧编码、STM32 parser/App、Mock BSP 与 ACK 返回同一 request token；验证 motor capability 缺失时拒绝 |
| Android `RemoteClient` host tests | PASS | fake TCP server；STM32 ACK stage/seq/result/CAPS 解析、超时降级、执行器不可用错误 |
| ManualDriveController | PASS | 按住刷新、松手 STOP、方向切换、急停、发送失败取消 |
| VoiceCommandMapper | PASS | 中文别名、否定语句保护、未知语句回退 |
| ESP32 API-stub syntax | PASS | 本地 ESP-IDF API stubs，不是 ESP-IDF target build |
| STM32 App/Mid + F103 BSP syntax | PASS | GCC C99 syntax，不是 Keil/MDK target build |
| ASan/UBSan | PASS | 主机可测模块 |
| GCC `-fanalyzer` | PASS | 11 个生产 C 翻译单元及启用执行器分支 |
| 板级映射审计 | PASS | 8 项单元测试；合成 fixture 不代表实物 PCB |
| Cortex-M3 safe-default link | PASS | Flash 8,684 B / 64 KB；RAM 1,400 B / 20 KB |
| 独立 actuator bench link | PASS | Flash 4,544 B / 64 KB；RAM 1,324 B / 20 KB；非生产整车固件 |
| AndroidManifest XML | PASS | XML parser |
| `git diff --check` | PASS | 空白/补丁格式检查 |
| import script `py_compile` | PASS | Python 语法检查 |
| V1.4 GitHub import script | PASS | 本地 bare-remote 模拟；171 个文件导入，远端 SHA 核对，模拟 `main` 保持不变 |

完整日志见仓库根目录 `TEST_RUN_LOG_V1.4.txt`。

## 3. 响应语义

- `stage=stm32_app`：收到与请求匹配的 STM32 App ACK。`result=0` 为已处理，`result=1` 为重复帧，`result=2` 为执行器后端不可用（返回 `ok=false`）。
- `stage=esp32_queued`：ESP32 已接受入队，但在 250ms 等待窗口内没有匹配 ACK；`warning=stm32_ack_timeout`。这不是 STM32 执行确认。
- `capabilities` 是编译期后端能力声明。即使 STM32 App ACK 成功，也不证明电机已转动、舵机已到位或车辆实际移动。
- ACK 映射表容量 16、记录超时 3 秒；表满时仍发送命令，但不关联本次 ACK，手机收到未确认状态。该退化路径不会阻塞 STOP。

## 4. 未验证项

| 项目 | 状态 | 原因 |
|---|---|---|
| Android Gradle `assembleDebug` / APK | NOT RUN | 当前环境无 Gradle 命令、Android SDK 和 Gradle Wrapper JAR |
| Android Activity/Framework SDK 编译 | NOT RUN | 需要真实 Android SDK/Gradle 构建 |
| ESP-IDF `esp32s3` target build / flash | NOT RUN | 当前环境未配置 ESP-IDF 工具链 |
| STM32 Keil/MDK target build / flash | NOT RUN | 当前环境未运行 Keil/MDK |
| PB10 → ESP32 GPIO4 实物 ACK 波形 | NOT RUN | 未连接实物板卡/逻辑分析仪 |
| Android 手机 ↔ ESP32 SoftAP/TCP | NOT RUN | 未进行真机网络联调 |
| 电机/舵机实际动作与反馈 | NOT RUN | 未做整车台架/实物验收；生产 capability 仍默认关闭 |

## 5. 安全边界

STM32 ACK 是 App 层处理结果，不是执行器传感器反馈。生产构建的 `VC_ACTUATOR_OUTPUTS_ENABLE` 默认关闭，motor/servo capability 位保持 0；不要为了让 UI 显示成功而打开输出门。首次硬件测试应抬起驱动轮、使用限流电源，并准备可直接断电的物理急停。TCP 控制端仍没有应用层认证，仅可在隔离网络演示并修改默认 SoftAP 密码。