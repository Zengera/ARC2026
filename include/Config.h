#pragma once

#include <Arduino.h>

// ==========================================
// GPIO割り当て
// ==========================================
// モータドライバ
constexpr uint8_t PIN_MOTOR_LEFT_A  = 18;
constexpr uint8_t PIN_MOTOR_LEFT_B  = 19;
constexpr uint8_t PIN_MOTOR_RIGHT_A = 22;
constexpr uint8_t PIN_MOTOR_RIGHT_B = 23;

// 赤外線受信機
constexpr uint8_t PIN_IR_RX_HIGH = 2; // IR2 (高優先)
constexpr uint8_t PIN_IR_RX_LOW  = 4; // IR4 (低優先)

// ==========================================
// PWM制御仕様
// ==========================================
constexpr uint32_t PWM_FREQ       = 20000; // 20kHz
constexpr uint8_t  PWM_RESOLUTION = 8;     // 8bit (0〜255)
constexpr int16_t  MOTOR_MAX_PWM  = 255;

// ==========================================
// 制御パラメータ
// ==========================================
constexpr uint32_t CONTROL_CYCLE_MS = 10;   // 10ms周期
constexpr uint32_t LOG_CYCLE_MS     = 1000; // 1秒周期

// PS4コントローラ
constexpr int8_t   PS4_DEADZONE     = 15;   // デッドゾーン ±15

// IRタイムアウト
constexpr uint32_t IR_SIGNAL_TIMEOUT_MS = 150; // 150ms信号途絶で停止
constexpr uint32_t IR_RELEASE_DELAY_MS  = 500; // さらに500ms後にPS4へ復帰

// IR走行時のモータ出力デューティ
constexpr int16_t  IR_DRIVE_SPEED       = 200;

// シリアル通信速度
constexpr uint32_t SERIAL_BAUD_RATE     = 115200;

// PS4ペアリング用Bluetooth MACアドレス
// 空文字列 "" の場合はESP32のハードウェアMACアドレスを使用
constexpr const char* PS4_BT_MAC        = "e0:8c:fe:fe:a6:2e";

