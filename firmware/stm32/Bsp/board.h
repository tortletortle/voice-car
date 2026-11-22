/**
 * Board constants — hardware from local bringup/wiring (2026-11-15 confirm).
 * Software contract follows GitHub voice-car V0.3 design.
 *
 * MCU: STM32F103C8T6 card (U1) · ESP32-S3 DevKit (U2)
 * Motors: 2× TTL via TB6612-style dual bridge on J4A/J4B
 *
 * Chip / clock / ports (acceptance):
 *   STM32F103C8T6 · bringup HSI 8 MHz · USART3 PB10/PB11 @ 115200
 *   OLED soft-I2C PB6/PB7 · MotA/MotB only · SERVO PWM gated off by default
 */
#ifndef BOARD_H
#define BOARD_H

/* ========== 板间 UART：U1.35/36 ↔ U2 GPIO4/5 ==========
 * 网名 ESP32_RX = 进 ESP32 的 RX（STM32 TX 对端）= PB10 USART3_TX
 * 网名 ESP32_TX = 出 ESP32 的 TX（STM32 RX 对端）= PB11 USART3_RX
 */
#define LINK_UART_BAUD       115200u
#define LINK_USART           3
#define LINK_TX_PIN_PB       10u
#define LINK_RX_PIN_PB       11u
#define LINK_RX_RING_SIZE    512u

/* ========== 双轮 = 只用 A/B 通道；C/D 本版不用 ==========
 * 左轮 MotA: AIN1, AIN2, PWA
 * 右轮 MotB: BIN1, BIN2, PWB
 * 共用: STBY（运动前必须拉高，停止可拉低）
 * MCU↔TB6612 具体 GPIO：以 PCB 网表为准，未确认前 motor backend fail-closed
 */
#define MOTOR_USE_CH_A       1
#define MOTOR_USE_CH_B       1
#define MOTOR_USE_CH_C       0
#define MOTOR_USE_CH_D       0

#define MOTOR_SWAP_LR        0
#define MOTOR_INVERT_L       0
#define MOTOR_INVERT_R       0
#define MOTOR_STBY_ACTIVE_HIGH 1
#define MOTOR_GPIO_CONFIGURED  0  /* 0 = fail-closed；PCB 对脚后改 1 */

/* --- motion: timed auto-stop；共 5V 先低 PWM --- */
#define MOTION_DEFAULT_MS    1500u
#define MOTION_TURN_MS       800u
#define MOTION_PWM_DEFAULT   30u

/* --- gimbal 硬件实测 ---
 * SERVO1 (ch1 / PA0)：俯仰定位角
 *   0° = 低头；-45° = 抬头；行程 [-45, 0]（抬头必须是负号，不是 +45）
 *   UP→更负朝 -45，DOWN→更正朝 0
 *   脉宽（2026-11-22 手调确认）：抬头 500µs，低头 1000µs
 * SERVO2 (ch2 / PA1)：偏航定位舵机（不标度数）
 *   左=1000µs，中=1500µs，右=2000µs
 *   LEFT/RIGHT=定住该侧；YAW_STOP=回中；YAW_SHAKE=摇头扫角后回中
 *   PITCH_NOD=点头：低头→抬头→回中（与摇头互斥，只用已手调 µs，避免 PWM 乱跳）
 */
#define GIMBAL_PITCH_CH      1  /* SERVO1 */
#define GIMBAL_YAW_CH        2  /* SERVO2 */
#define GIMBAL_PITCH_DOWN    0     /* 低头 */
#define GIMBAL_PITCH_UP      (-45) /* 抬头 */
#define GIMBAL_PITCH_MIN     GIMBAL_PITCH_UP
#define GIMBAL_PITCH_MAX     GIMBAL_PITCH_DOWN
#define GIMBAL_PITCH_CENTER  (-22)
#define GIMBAL_STEP_DEG      5
#define GIMBAL_PITCH_DOWN_US   1000u /* 0° 低头 */
#define GIMBAL_PITCH_UP_US      500u /* -45° 抬头 */
#define GIMBAL_PITCH_CENTER_US  750u /* -22° 中点近似 */

/* SERVO2 偏航：定位脉宽（µs），不标度数 */
#define GIMBAL_YAW_LEFT_US    1000u
#define GIMBAL_YAW_CENTER_US  1500u
#define GIMBAL_YAW_RIGHT_US   2000u
#define GIMBAL_YAW_STOP_US    GIMBAL_YAW_CENTER_US /* 回中 */
#define GIMBAL_YAW_SHAKE_HOLD_MS 400u /* 摇头每一侧停留 */
#define GIMBAL_PITCH_NOD_HOLD_MS 450u /* 点头每一段停留：低头/抬头/回中 */

/* 舵机 PWM 默认关闭：本地 bringup 确认 SERVO OFF，待 PCB 网表再开 */
#ifndef VC_SERVO_OUTPUT_ENABLE
#define VC_SERVO_OUTPUT_ENABLE 1  /* 2026-11-22 用户确认打开并测 SERVO2 */
#endif

/* OLED：本地实测脚 PB6/PB7；UI 走 V0.3 四行状态屏 */
#define OLED_ENABLED         1
#define OLED_SCL_PIN_PB      6u
#define OLED_SDA_PIN_PB      7u
#define OLED_ADDR_PRIMARY    0x3Cu
#define OLED_ADDR_FALLBACK   0x3Du
#ifndef OLED_COLUMN_OFFSET
#define OLED_COLUMN_OFFSET   0  /* SH1106 可视区偏移时改 2 */
#endif

#endif /* BOARD_H */
