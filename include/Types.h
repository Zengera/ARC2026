#pragma once

#include <Arduino.h>

// 制御ソース状態 (Architecture.md 準拠)
enum class ControlSource {
    NONE,
    IR2,
    IR4,
    PS4
};

// モーションコマンド (Architecture.md 準拠)
enum class MotionCommand {
    COAST_STOP,
    BRAKE,
    FORWARD,
    BACKWARD,
    TURN_LEFT,
    TURN_RIGHT,
    MIXED_DRIVE
};

// モータ出力
struct MotorOutput {
    int16_t leftSpeed  = 0; // -255 〜 255
    int16_t rightSpeed = 0; // -255 〜 255
};

// PS4入力状態
struct PS4InputData {
    int16_t throttle    = 0; // -255 〜 255 (前後)
    int16_t steering    = 0; // -255 〜 255 (旋回)
    int8_t  rawLY       = 0; // スティック生値 (-128 〜 127)
    int8_t  rawRX       = 0; // スティック生値 (-128 〜 127)
    bool    connected   = false;
    bool    brakeButton = false; // R1ボタン
};

// IR受信状態
struct IRInputData {
    bool          hasSignal = false; // 150ms以内
    bool          inControl = false; // 途絶後650ms以内
    MotionCommand command   = MotionCommand::COAST_STOP;
    uint32_t      rawCode   = 0;
};
