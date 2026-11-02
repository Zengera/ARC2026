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

    // デフォルトの加減速度パラメータ (PWM単位 / 秒)
    // 0 -> 255: 約0.50秒 (510 PWM/s)
    // 255 -> 0: 約0.30秒 (850 PWM/s)
    static constexpr float DEFAULT_ACCELERATION = 510.0f;
    static constexpr float DEFAULT_DECELERATION = 850.0f;

    // 最低PWMのデフォルト値 (25% = 64)
    static constexpr uint8_t DEFAULT_MIN_PWM = 64;

    MotorController();
    ~MotorController() = default;

    /**
     * @brief モータドライバ（LEDC PWMおよびGPIO）の初期化
     */
    void begin();

    /**
     * @brief 台形加減速の更新処理（周期呼び出し）
     */
    void update();

    /**
     * @brief 左右のモータ目標速度を設定 (-255 〜 +255)
     * @note 台形加減速により徐々に目標速度へ追従します。
     */
    void setTargetSpeeds(int16_t left, int16_t right);

    /**
     * @brief 左右のモータ速度を指定 (-255 〜 +255)
     * @note 互換性のため setTargetSpeeds() と update() を実行します。
     */
    void setSpeeds(int16_t left, int16_t right);

    /**
     * @brief 前進 (目標速度: 1〜255)
     */
    void forward(uint8_t speed = 255);

    /**
     * @brief 後退 (目標速度: 1〜255)
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
     * @brief ショートブレーキ (HH) - 加減速を即時解除し短絡ブレーキ
     */
    void brake();

    /**
     * @brief 惰性停止 (LL) - 目標速度0へ台形減速
     */
    void coastStop();

    /**
     * @brief 即時停止 (加減速をバイパスして即座にLL停止)
     */
    void stopImmediate();

    /**
     * @brief モーションコマンドに従ってモータを駆動
     */
    void executeCommand(MotionCommand cmd, int16_t left_speed = 0, int16_t right_speed = 0);

    /**
     * @brief 現在の実出力速度を取得 (-255 〜 +255)
     */
    int16_t getLeftSpeed() const { return m_leftSpeed; }
    int16_t getRightSpeed() const { return m_rightSpeed; }

    /**
     * @brief 現在の目標速度を取得 (-255 〜 +255)
     */
    int16_t getTargetLeftSpeed() const { return m_targetLeftSpeed; }
    int16_t getTargetRightSpeed() const { return m_targetRightSpeed; }

    /**
     * @brief 加速度・減速度の設定 (PWM / 秒)
     */
    void setAcceleration(float accel) { m_accelRate = accel; }
    void setDeceleration(float decel) { m_decelRate = decel; }
    float getAcceleration() const { return m_accelRate; }
    float getDeceleration() const { return m_decelRate; }

    /**
     * @brief 台形加減速の最低PWMを設定・取得 (0〜255, 初期値: 64 [25%])
     */
    void setMinPwm(uint8_t minPwm) { m_minPwm = minPwm; }
    uint8_t getMinPwm() const { return m_minPwm; }

    /**
     * @brief 台形加減速制御の有効/無効切り替え
     */
    void setRampEnabled(bool enabled) { m_rampEnabled = enabled; }
    bool isRampEnabled() const { return m_rampEnabled; }

private:
    void setChannelDuty(ledc_channel_t channel, uint32_t duty);
    void applyPwm(int16_t left, int16_t right);
    float stepRamp(float current, int16_t target, float dt);

    int16_t m_targetLeftSpeed{0};
    int16_t m_targetRightSpeed{0};
    float m_currentLeftSpeed{0.0f};
    float m_currentRightSpeed{0.0f};

    int16_t m_leftSpeed{0};
    int16_t m_rightSpeed{0};
    int32_t m_channelDuty[4]{-1, -1, -1, -1};

    float m_accelRate{DEFAULT_ACCELERATION};
    float m_decelRate{DEFAULT_DECELERATION};
    uint8_t m_minPwm{DEFAULT_MIN_PWM};
    int64_t m_lastUpdateTimeUs{0};
    bool m_rampEnabled{true};
    bool m_isBraking{false};
};

#endif // MOTOR_CONTROLLER_H
