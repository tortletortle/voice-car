# Voice Car Android App V1.4.0 — STM32 ACK 状态回传

日期：2026-11-22
基线：`integration/v1.3-android-voice-commands`

## 本轮目标

让手机遥控端不再把 ESP32 队列入队应答误认为 STM32 已处理。复用既有 STM32→ESP32 9 字节 ACK，通过 request token、UART 序号/命令匹配和同步 TCP JSONL 响应，把 STM32 App 处理结果返回给手机。

## 变更

### 1. ESP32 板间链路关联

- 新增 `link_stm32_send_tracked(cmd, request_id)`；request ID 只存在 ESP32 软件层，不改变 `AA 55 CMD LEN SEQ CHK` 六字节命令帧。
- 发送时保存 request ID、SEQ、CMD；ACK 必须同时匹配 SEQ 和 CMD 才能完成请求。
- 最多保留 16 条待确认记录，3 秒过期；表满时仍发送命令，但不建立 ACK 关联，避免因跟踪表满阻塞 STOP。
- 未关联的 NOP 心跳 ACK、错误 CMD ACK 和过期 ACK 不会被交给手机请求。

### 2. 命令路由与 TCP 应答

- 路由事件携带 request ID；完成队列将 ACK 送回 TCP 请求任务。
- 手机命令最多等待 STM32 ACK 250ms。
- 匹配 ACK 返回：`ok=true`、`stage=stm32_app`、`seq`、`result`、`capabilities`。
- STM32 返回 `ACTUATOR_UNAVAILABLE` 时，ESP32 返回 `ok=false` 与 `error=actuator_unavailable`，手机显示拒绝原因和 capability 位。
- ACK 超时返回 `ok=true`、`stage=esp32_queued`、`warning=stm32_ack_timeout`；这只确认 ESP32 已接受入队，不宣称 STM32 已处理。
- 纯播报事件没有 STM32 命令，不等待 STM32 ACK。

### 3. Android 状态呈现

`RemoteClient.sendAction` 返回 `RemoteActionResult`，解析 stage、seq、result、capabilities 和 warning。界面状态区分：

- STM32 App 已处理（软件 ACK）；
- STM32 报告重复帧（未重复执行）；
- 仅 ESP32 入队，未收到 STM32 ACK；
- 执行器后端未启用。

所有成功状态均明确说明：软件 ACK 不代表电机已经转动或舵机已经到位。

## 自动化验证

执行 `./tests/run_all_checks.sh`。新增/扩展测试：

- `test_link_stm32_tracking`：乱序 ACK 关联、SEQ+CMD 双重匹配、迟到 ACK 过期、NOP 心跳隔离。
- `test_cmd_router_integration`：request token 从路由器进入链路并返回完成队列。
- `test_full_command_pipeline`：真实六字节编码器、STM32 parser/App 与 Mock BSP 之间的 request→ACK 往返；验证执行器能力缺失时返回拒绝。
- Kotlin TCP 主机测试：解析 `stm32_app` ACK、序号、结果、CAPS；验证 ACK 超时状态与未确认提示。
- ESP32 API stub 语法检查、STM32 syntax、ASan/UBSan、GCC `-fanalyzer`、Cortex-M3 safe-default 与 actuator bench 链接。

## 当前验证边界

- 主机测试覆盖 ACK 协议、请求关联、App 处理结果和 Android 解析；ESP32 UART 物理收发、TCP 实机响应仍未验证。
- Android Kotlin helper tests 通过，但当前环境没有 Android SDK/Gradle，`assembleDebug`、Activity SDK 编译、APK 安装未运行。
- 没有实测 PB10→ESP32 GPIO4 ACK 波形、真实手机 Wi-Fi 往返、实物电机/舵机动作。
- STM32 ACK 只代表 App 层处理，不是执行器物理反馈。当前生产执行器 capability 默认仍关闭；缺少后端时移动/头部动作应被明确拒绝。
- TCP 控制仍没有应用层认证；仅在私有隔离网络中演示，并更换默认 SoftAP 密码。