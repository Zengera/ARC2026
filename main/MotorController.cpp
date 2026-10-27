#include "MotorController.h"
#include "esp_log.h"
#include <algorithm>

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

    coastStop();
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

void MotorController::setSpeeds(int16_t left, int16_t right)
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
    m_leftSpeed  = 0;
    m_rightSpeed = 0;
    setChannelDuty(LEDC_CHANNEL_LEFT_A, 255);
    setChannelDuty(LEDC_CHANNEL_LEFT_B, 255);
    setChannelDuty(LEDC_CHANNEL_RIGHT_A, 255);
    setChannelDuty(LEDC_CHANNEL_RIGHT_B, 255);
}

void MotorController::coastStop()
{
    m_leftSpeed  = 0;
    m_rightSpeed = 0;
    setChannelDuty(LEDC_CHANNEL_LEFT_A, 0);
    setChannelDuty(LEDC_CHANNEL_LEFT_B, 0);
    setChannelDuty(LEDC_CHANNEL_RIGHT_A, 0);
    setChannelDuty(LEDC_CHANNEL_RIGHT_B, 0);
}

void MotorController::executeCommand(MotionCommand cmd, int16_t left_speed, int16_t right_speed)
{
    switch (cmd) {
    case MotionCommand::FORWARD:
        forward(255);
        break;
    case MotionCommand::BACKWARD:
        backward(255);
        break;
    case MotionCommand::TURN_LEFT:
        turnLeft(255);
        break;
    case MotionCommand::TURN_RIGHT:
        turnRight(255);
        break;
    case MotionCommand::BRAKE:
        brake();
        break;
    case MotionCommand::MIXED_DRIVE:
        setSpeeds(left_speed, right_speed);
        break;
    case MotionCommand::COAST_STOP:
    default:
        coastStop();
        break;
    }
}
