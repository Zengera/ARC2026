#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <cstdint>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "CommonTypes.h"

class MotorController
{
public:
    static constexpr gpio_num_t PIN_LEFT_A  = GPIO_NUM_18;
    static constexpr gpio_num_t PIN_LEFT_B  = GPIO_NUM_19;
    static constexpr gpio_num_t PIN_RIGHT_A = GPIO_NUM_22;
    static constexpr gpio_num_t PIN_RIGHT_B = GPIO_NUM_23;

    MotorController();
    ~MotorController() = default;

    /**
     * @brief モータドライバ（LEDC PWMおよびGPIO）の初期化
     */
    void begin();

    /**
     * @brief 左右のモータ速度を直接指定 (-255 〜 +255)
     */
    void setSpeeds(int16_t left, int16_t right);

    /**
     * @brief 前進 (速度: 1〜255)
     */
    void forward(uint8_t speed = 255);

    /**
     * @brief 後退 (速度: 1〜255)
     */
    void backward(uint8_t speed = 255);

    /**
     * @brief 左旋回
     */
    void turnLeft(uint8_t speed = 255);

    /**
     * @brief 右旋回
     */
    void turnRight(uint8_t speed = 255);

    /**
     * @brief ショートブレーキ (HH)
     */
    void brake();

    /**
     * @brief 惰性停止 (LL)
     */
    void coastStop();

    /**
     * @brief モーションコマンドに従ってモータを駆動
     */
    void executeCommand(MotionCommand cmd, int16_t left_speed = 0, int16_t right_speed = 0);

    int16_t getLeftSpeed() const { return m_leftSpeed; }
    int16_t getRightSpeed() const { return m_rightSpeed; }

private:
    void setChannelDuty(ledc_channel_t channel, uint32_t duty);

    int16_t m_leftSpeed{0};
    int16_t m_rightSpeed{0};
    int32_t m_channelDuty[4]{-1, -1, -1, -1};
};

#endif // MOTOR_CONTROLLER_H
