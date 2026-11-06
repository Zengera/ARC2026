#include "MotorController.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <algorithm>
#include <cmath>

static const char *TAG = "MotorController";

#define LEDC_SPEED_MODE         LEDC_HIGH_SPEED_MODE
#define LEDC_TIMER_NUM          LEDC_TIMER_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          20000

#define LEDC_CHANNEL_LEFT_A     LEDC_CHANNEL_0
#define LEDC_CHANNEL_LEFT_B     LEDC_CHANNEL_1
#define LEDC_CHANNEL_RIGHT_A    LEDC_CHANNEL_2
#define LEDC_CHANNEL_RIGHT_B    LEDC_CHANNEL_3

MotorController::MotorController() {}

void MotorController::begin()
{
    ESP_LOGI(TAG, "Init MotorController: Left(%d,%d), Right(%d,%d)",
             PIN_LEFT_A, PIN_LEFT_B, PIN_RIGHT_A, PIN_RIGHT_B);

    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode       = LEDC_SPEED_MODE;
    timer_cfg.duty_resolution  = LEDC_DUTY_RES;
    timer_cfg.timer_num        = LEDC_TIMER_NUM;
    timer_cfg.freq_hz          = LEDC_FREQUENCY;
    timer_cfg.clk_cfg          = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    const gpio_num_t pins[4] = { PIN_LEFT_A, PIN_LEFT_B, PIN_RIGHT_A, PIN_RIGHT_B };
    const ledc_channel_t chs[4] = {
        LEDC_CHANNEL_LEFT_A, LEDC_CHANNEL_LEFT_B,
        LEDC_CHANNEL_RIGHT_A, LEDC_CHANNEL_RIGHT_B
    };

    for (int i = 0; i < 4; i++) {
        ledc_channel_config_t ch_cfg = {};
        ch_cfg.gpio_num   = pins[i];
        ch_cfg.speed_mode = LEDC_SPEED_MODE;
        ch_cfg.channel    = chs[i];
        ch_cfg.intr_type  = LEDC_INTR_DISABLE;
        ch_cfg.timer_sel  = LEDC_TIMER_NUM;
        ch_cfg.duty       = 0;
        ch_cfg.hpoint     = 0;
        ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
        m_channelDuty[i] = 0;
    }

    stopImmediate();
}

void MotorController::setChannelDuty(ledc_channel_t channel, uint32_t duty)
{
    if (channel < 4 && m_channelDuty[channel] == static_cast<int32_t>(duty)) {
        return;
    }
    if (channel < 4) {
        m_channelDuty[channel] = static_cast<int32_t>(duty);
    }
    ledc_set_duty(LEDC_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_SPEED_MODE, channel);
}

void MotorController::applyPwm(int16_t left, int16_t right)
{
    left  = std::clamp<int16_t>(left, -255, 255);
    right = std::clamp<int16_t>(right, -255, 255);

    m_leftSpeed  = left;
    m_rightSpeed = right;

    // 左モータ (A: PWM, B: LOW / A: LOW, B: PWM / A: LOW, B: LOW)
    if (left > 0) {
        setChannelDuty(LEDC_CHANNEL_LEFT_A, static_cast<uint32_t>(left));
        setChannelDuty(LEDC_CHANNEL_LEFT_B, 0);
    } else if (left < 0) {
        setChannelDuty(LEDC_CHANNEL_LEFT_A, 0);
        setChannelDuty(LEDC_CHANNEL_LEFT_B, static_cast<uint32_t>(-left));
    } else {
        setChannelDuty(LEDC_CHANNEL_LEFT_A, 0);
        setChannelDuty(LEDC_CHANNEL_LEFT_B, 0);
    }

    // 右モータ
    if (right > 0) {
        setChannelDuty(LEDC_CHANNEL_RIGHT_A, static_cast<uint32_t>(right));
        setChannelDuty(LEDC_CHANNEL_RIGHT_B, 0);
    } else if (right < 0) {
        setChannelDuty(LEDC_CHANNEL_RIGHT_A, 0);
        setChannelDuty(LEDC_CHANNEL_RIGHT_B, static_cast<uint32_t>(-right));
    } else {
        setChannelDuty(LEDC_CHANNEL_RIGHT_A, 0);
        setChannelDuty(LEDC_CHANNEL_RIGHT_B, 0);
    }
}

float MotorController::stepRamp(float current, int16_t target, float dt)
{
    // 非ゼロの目標速度が最低PWM未満の場合は最低PWMへ補正（不感帯補正）
    if (m_minPwm > 0) {
        if (target > 0 && target < m_minPwm) {
            target = m_minPwm;
        } else if (target < 0 && target > -static_cast<int16_t>(m_minPwm)) {
            target = -static_cast<int16_t>(m_minPwm);
        }
    }

    const float targetF = static_cast<float>(target);
    const float minPwmF = static_cast<float>(m_minPwm);

    if (std::abs(current - targetF) < 0.01f) {
        return targetF;
    }

    // ゼロ停止状態からの加速判定
    if (std::abs(current) < 0.001f) {
        if (targetF > 0.0f) {
            // 正方向への加速開始: 最低PWMからスタート
            float startSpeed = (minPwmF > 0.0f) ? minPwmF : 0.0f;
            float next = startSpeed + m_accelRate * dt;
            return (next > targetF) ? targetF : next;
        } else if (targetF < 0.0f) {
            // 負方向への加速開始: -最低PWMからスタート
            float startSpeed = (minPwmF > 0.0f) ? -minPwmF : 0.0f;
            float next = startSpeed - m_accelRate * dt;
            return (next < targetF) ? targetF : next;
        } else {
            return 0.0f;
        }
    } else if (current > 0.0f) {
        // 現在前進中
        if (targetF > current) {
            // 前進方向への加速
            float next = current + m_accelRate * dt;
            return (next > targetF) ? targetF : next;
        } else if (targetF >= minPwmF) {
            // 最低PWM以上の目標値へ減速
            float next = current - m_decelRate * dt;
            return (next < targetF) ? targetF : next;
        } else {
            // targetF < minPwmF (0へ停止減速、または逆転への反転)
            float next = current - m_decelRate * dt;
            // 最低PWMを下回ったら0へスナップ（モータ停止または反転デッドタイム）
            if (next < minPwmF) {
                return 0.0f;
            }
            return next;
        }
    } else {
        // 現在後退中 (current < 0.0f)
        if (targetF < current) {
            // 後退方向への加速
            float next = current - m_accelRate * dt;
            return (next < targetF) ? targetF : next;
        } else if (targetF <= -minPwmF) {
            // -最低PWM以下の目標値へ減速
            float next = current + m_decelRate * dt;
            return (next > targetF) ? targetF : next;
        } else {
            // targetF > -minPwmF (0へ停止減速、または正転への反転)
            float next = current + m_decelRate * dt;
            // -最低PWMを上回ったら0へスナップ（モータ停止または反転デッドタイム）
            if (next > -minPwmF) {
                return 0.0f;
            }
            return next;
        }
    }
}

void MotorController::update()
{
    if (m_isBraking) {
        return;
    }

    int16_t targetLeft  = m_targetLeftSpeed;
    int16_t targetRight = m_targetRightSpeed;
    if (m_minPwm > 0) {
        if (targetLeft > 0 && targetLeft < m_minPwm) {
            targetLeft = m_minPwm;
        } else if (targetLeft < 0 && targetLeft > -static_cast<int16_t>(m_minPwm)) {
            targetLeft = -static_cast<int16_t>(m_minPwm);
        }
        if (targetRight > 0 && targetRight < m_minPwm) {
            targetRight = m_minPwm;
        } else if (targetRight < 0 && targetRight > -static_cast<int16_t>(m_minPwm)) {
            targetRight = -static_cast<int16_t>(m_minPwm);
        }
    }
    m_currentLeftSpeed  = static_cast<float>(targetLeft);
    m_currentRightSpeed = static_cast<float>(targetRight);
    applyPwm(targetLeft, targetRight);
    return;
}

void MotorController::setTargetSpeeds(int16_t left, int16_t right)
{
    m_isBraking = false;
    m_targetLeftSpeed  = std::clamp<int16_t>(left, -250, 250);
    m_targetRightSpeed = std::clamp<int16_t>(right, -255, 255);
}

void MotorController::setSpeeds(int16_t left, int16_t right)
{
    setTargetSpeeds(left, right);
    update();
}

void MotorController::forward(uint8_t speed)
{
    setSpeeds(speed, speed);
}

void MotorController::backward(uint8_t speed)
{
    setSpeeds(-static_cast<int16_t>(speed), -static_cast<int16_t>(speed));
}

void MotorController::turnLeft(uint8_t speed)
{
    setSpeeds(-static_cast<int16_t>(speed), static_cast<int16_t>(speed));
}

void MotorController::turnRight(uint8_t speed)
{
    setSpeeds(static_cast<int16_t>(speed), -static_cast<int16_t>(speed));
}

void MotorController::brake()
{
    m_isBraking         = true;
    m_targetLeftSpeed   = 0;
    m_targetRightSpeed  = 0;
    m_currentLeftSpeed  = 0.0f;
    m_currentRightSpeed = 0.0f;
    m_leftSpeed         = 0;
    m_rightSpeed        = 0;
    m_lastUpdateTimeUs  = esp_timer_get_time();

    // ショートブレーキ (HH)
    setChannelDuty(LEDC_CHANNEL_LEFT_A, 255);
    setChannelDuty(LEDC_CHANNEL_LEFT_B, 255);
    setChannelDuty(LEDC_CHANNEL_RIGHT_A, 255);
    setChannelDuty(LEDC_CHANNEL_RIGHT_B, 255);
}

void MotorController::coastStop()
{
    m_isBraking = false;
    setTargetSpeeds(0, 0);
    update();
}

void MotorController::stopImmediate()
{
    m_isBraking         = false;
    m_targetLeftSpeed   = 0;
    m_targetRightSpeed  = 0;
    m_currentLeftSpeed  = 0.0f;
    m_currentRightSpeed = 0.0f;
    m_lastUpdateTimeUs  = esp_timer_get_time();

    applyPwm(0, 0);
}

void MotorController::executeCommand(MotionCommand cmd, int16_t left_speed, int16_t right_speed)
{
    switch (cmd) {
    case MotionCommand::FORWARD:
        setTargetSpeeds(255, 255);
        break;
    case MotionCommand::BACKWARD:
        setTargetSpeeds(-255, -255);
        break;
    case MotionCommand::TURN_LEFT:
        setTargetSpeeds(-255, 255);
        break;
    case MotionCommand::TURN_RIGHT:
        setTargetSpeeds(255, -255);
        break;
    case MotionCommand::BRAKE:
        brake();
        return;
    case MotionCommand::MIXED_DRIVE:
        setTargetSpeeds(left_speed, right_speed);
        break;
    case MotionCommand::COAST_STOP:
    default:
        setTargetSpeeds(0, 0);
        break;
    }
    update();
}
