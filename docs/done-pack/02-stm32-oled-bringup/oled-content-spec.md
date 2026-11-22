# OLED 显示内容规范

Voice Car 0.96″ SSD1306（128×64）单页状态屏文案。只定「显示什么」；驱动 / I2C / 刷屏另案。

对齐命令：`firmware/common/cmd_protocol.h`。

## 1. 画布

| 项 | 取值 |
|---|---|
| 分辨率 | 128×64 |
| 字号 | ASCII 8×16 |
| 栅格 | **4 行 × 16 列**（每行最多 16 字符） |
| 语言 | 英文缩写；`VC` = Voice Car |
| 页数 | 单页，无菜单、无翻页 |

列号示意（等宽）：

```
列: 0123456789ABCDEF
```

## 2. 信息优先级

1. 当前运动态（车在干什么）
2. 最近一次命令（刚下了什么）
3. 云台姿态（俯仰 / 偏航）
4. 辅状态（PWM 档 + 链路字）

不放：电池%、WiFi、IP、调试 HEX、长句提示。

## 3. 固定四行布局

```
L0: VC  <MOTION_____>
L1: CMD <TAG>  S<seq>
L2: P <pp>  Y <yy>
L3: PWM <nn>  <LINK>
```

| 行 | 字段 | 格式 | 说明 |
|---|---|---|---|
| L0 | 品牌 + 运动态 | `VC` + 两空格 + 态名左对齐 | 态名见 §4；右侧空格补齐至 ≤16 |
| L1 | 最近命令 + 序号 | `CMD ` + 标签（≤4）+ 两空格 + `S` + 两位十六进制序号 | 尚无帧时序号为 `--` |
| L2 | 云台 | `P ` + 俯仰（2～3 位）+ 两空格 + `Y ` + 偏航 | 中心默认 90 / 90 |
| L3 | PWM + 链路 | `PWM ` + 两位百分数 + 两空格 + `OK` 或 `--` | PWM 取当前默认档（现 30） |

## 4. 运动态文案（L0）

| 内部状态 | 屏幕 | 何时出现 |
|---|---|---|
| 静止 / 自停后 | `IDLE` | 上电、`CMD_STOP`、限时运动结束 |
| 前进中 | `FWD` | `CMD_FWD` 且未到期 |
| 后退中 | `BACK` | `CMD_BACK` 且未到期 |
| 左转中 | `LEFT` | `CMD_LEFT` 且未到期 |
| 右转中 | `RIGHT` | `CMD_RIGHT` 且未到期 |

L0 **只反映车体是否在动**；云台命令不改 L0。

## 5. 命令标签字典（L1）

与 `vc_cmd_t` 一一对应：

| `vc_cmd_t` | 屏幕标签 |
|---|---|
| `CMD_STOP` | `STOP` |
| `CMD_FWD` | `FWD` |
| `CMD_BACK` | `BACK` |
| `CMD_LEFT` | `LEFT` |
| `CMD_RIGHT` | `RGHT` |
| `CMD_GIMBAL_UP` | `G-UP` |
| `CMD_GIMBAL_DOWN` | `G-DN` |
| `CMD_GIMBAL_LEFT` | `G-L` |
| `CMD_GIMBAL_RIGHT` | `G-R` |
| `CMD_GIMBAL_CENTER` | `G-C` |
| `CMD_NOP` / 未知 | `----` |

`RGHT` 故意四字：L0 运动态用全词 `RIGHT`，L1 命令标签用 `RGHT`，避免同行抢视觉。

## 6. 状态切换规则

```text
Boot → Idle（首帧 IDLE）
Idle ↔ Moving：FWD/BACK/LEFT/RIGHT 进入；限时到或 STOP 回到 Idle
Idle + 仅云台：保持 Idle，只改 L1/L2
Moving + 新运动命令：保持 Moving，刷新剩余时间与 L0/L1
```

细则：

- **任意有效命令**：立刻改 L1（标签 + seq）；运动命令同步改 L0；云台命令同步改 L2。
- **运动限时结束**：L0 → `IDLE`；L1 **保留**最后一条命令（回看「刚才说了什么」）。
- **链路字**：上电 `--`；首次成功解帧 → `OK`；本版不做超时掉回 `--`。
- **刷新观感**：内容变了下一刷周期内更新；静止画面不闪烁。

## 7. 样例帧

每行字符数标注在右侧（须 ≤16）。

### 7.1 上电 / 空闲

```
VC  IDLE         | 8
CMD ----  S--    | 13
P 90  Y 90       | 10
PWM 30  --       | 10
```

完整画面：

```
VC  IDLE
CMD ----  S--
P 90  Y 90
PWM 30  --
```

### 7.2 语音「前进」刚执行（seq=3）

```
VC  FWD          | 7
CMD FWD   S03    | 13
P 90  Y 90       | 10
PWM 30  OK       | 10
```

```
VC  FWD
CMD FWD   S03
P 90  Y 90
PWM 30  OK
```

### 7.3 前进自停后，又抬头云台（seq=7，P=100）

```
VC  IDLE         | 8
CMD G-UP  S07    | 13
P100  Y 90       | 10
PWM 30  OK       | 10
```

```
VC  IDLE
CMD G-UP  S07
P100  Y 90
PWM 30  OK
```

### 7.4 紧急停止（seq=0x0A）

```
VC  IDLE         | 8
CMD STOP  S0A    | 13
P100  Y 90       | 10
PWM 30  OK       | 10
```

```
VC  IDLE
CMD STOP  S0A
P100  Y 90
PWM 30  OK
```

## 8. 对照检查（实现前自检）

任选一条 `vc_cmd_t`，应能唯一推出：

| 命令类型 | L0 | L1 | L2 |
|---|---|---|---|
| 运动（FWD/BACK/LEFT/RIGHT） | 对应态名 | 对应标签 + 新 seq | 不变 |
| STOP | `IDLE` | `STOP` + 新 seq | 不变 |
| 云台 | 不变 | 对应 `G-*` + 新 seq | 角度按步进/限幅更新 |
| NOP / 未知 | 不变 | `----`（若仍显示） | 不变 |

## 9. 本规范不做

驱动代码、软 I2C、Keil 工程改动、中文字库、多页 UI、电池/网络图标。
