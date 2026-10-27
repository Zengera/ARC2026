#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <cstdint>

/**
 * @brief 制御ソース状態
 */
enum class ControlSource
{
    NONE,
    IR2,
    IR4,
    PS4
};

/**
 * @brief モーションコマンド
 */
enum class MotionCommand
{
    COAST_STOP,
    BRAKE,
    FORWARD,
    BACKWARD,
    TURN_LEFT,
    TURN_RIGHT,
    MIXED_DRIVE
};

#endif // COMMON_TYPES_H
