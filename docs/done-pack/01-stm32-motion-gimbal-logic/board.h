/**
 * Board constants — frozen policy 2026-11-15 (user confirm pass).
 *
 * MCU: STM32F103C8T6 card (U1) · ESP32-S3 DevKit (U2)
 * Motors: 2× TTL via TB6612-style dual bridge on J4A/J4B
 */
#ifndef BOARD_H
#define BOARD_H

/* ========== 板间 UART：U1.35/36 ↔ U2 GPIO4/5 ==========
 * 网名 ESP32_RX = 进 ESP32 的 RX（STM32 TX 对端）
 * 网名 ESP32_TX = 出 ESP32 的 TX（STM32 RX 对端）
 * STM32 具体外设脚以最小系统卡丝印为准（连接器脚≠PA9 盲猜）
 */
#define LINK_UART_BAUD       115200u

/* ========== 双轮 = 只用 A/B 通道；C/D 本版不用 ==========
 * 左轮 MotA: AIN1, AIN2, PWA
 * 右轮 MotB: BIN1, BIN2, PWB
 * 共用: STBY（运动前必须拉高，停止可拉低）
 * 不用: CIN1/2, DIN1/2, PWC, PWD（软件不驱动）
 */
#define MOTOR_USE_CH_A       1
#define MOTOR_USE_CH_B       1
#define MOTOR_USE_CH_C       0
#define MOTOR_USE_CH_D       0

#define MOTOR_SWAP_LR        0   /* 实装反了改 1 */
#define MOTOR_INVERT_L       0
#define MOTOR_INVERT_R       0

/* STBY：1=使能驱动桥；上电默认 0，有运动命令才置 1 */
#define MOTOR_STBY_ACTIVE_HIGH 1

/* --- motion: timed auto-stop；共 5V 先低 PWM --- */
#define MOTION_DEFAULT_MS    1500u
#define MOTION_TURN_MS       800u
#define MOTION_PWM_DEFAULT   30u /* percent；电机+舵机共 5V，联调 ≤30 */

/* --- gimbal SERVO1/2 --- */
#define GIMBAL_PITCH_CH      1
#define GIMBAL_YAW_CH        2
#define GIMBAL_PITCH_CENTER  90
#define GIMBAL_YAW_CENTER    90
#define GIMBAL_PITCH_MIN     45
#define GIMBAL_PITCH_MAX     135
#define GIMBAL_YAW_MIN       45
#define GIMBAL_YAW_MAX       135
#define GIMBAL_STEP_DEG      10

/* OLED O1：硬件有座，固件 v0 不做（Phase 2） */
#define OLED_ENABLED         0

#endif /* BOARD_H */
