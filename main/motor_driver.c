#include "motor_driver.h"
#include "esp_log.h"

static const char *TAG = "MOTOR_DRIVER";

// デフォルトGPIO定義
#define DEFAULT_GPIO_LEFT_IN1   GPIO_NUM_18
#define DEFAULT_GPIO_LEFT_IN2   GPIO_NUM_19
#define DEFAULT_GPIO_RIGHT_IN1  GPIO_NUM_22
#define DEFAULT_GPIO_RIGHT_IN2  GPIO_NUM_23

// LEDC (PWM) 定義
#define LEDC_SPEED_MODE         LEDC_LOW_SPEED_MODE
#define LEDC_TIMER_NUM          LEDC_TIMER_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // 分解能: 0 〜 255
#define LEDC_FREQUENCY          20000             // PWM周波数 20kHz

// 各モータピン用のLEDCチャンネル
#define LEDC_CHANNEL_LEFT_IN1   LEDC_CHANNEL_0
#define LEDC_CHANNEL_LEFT_IN2   LEDC_CHANNEL_1
#define LEDC_CHANNEL_RIGHT_IN1  LEDC_CHANNEL_2
#define LEDC_CHANNEL_RIGHT_IN2  LEDC_CHANNEL_3

static motor_config_t s_motor_cfg;

static void set_ledc_duty(ledc_channel_t channel, uint32_t duty) {
    ledc_set_duty(LEDC_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_SPEED_MODE, channel);
}

void motor_driver_init(const motor_config_t *config) {
    if (config != NULL) {
        s_motor_cfg = *config;
    } else {
        s_motor_cfg.left_in1_gpio  = DEFAULT_GPIO_LEFT_IN1;
        s_motor_cfg.left_in2_gpio  = DEFAULT_GPIO_LEFT_IN2;
        s_motor_cfg.right_in1_gpio = DEFAULT_GPIO_RIGHT_IN1;
        s_motor_cfg.right_in2_gpio = DEFAULT_GPIO_RIGHT_IN2;
    }

    ESP_LOGI(TAG, "Initializing Motor Driver pins: Left(IN1:%d, IN2:%d), Right(IN1:%d, IN2:%d)",
             s_motor_cfg.left_in1_gpio, s_motor_cfg.left_in2_gpio,
             s_motor_cfg.right_in1_gpio, s_motor_cfg.right_in2_gpio);

    // LEDC タイマー設定
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_SPEED_MODE,
        .timer_num        = LEDC_TIMER_NUM,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // LEDC チャンネル設定（各GPIOをPWM制御可能に）
    ledc_channel_config_t ledc_channels[4] = {
        {
            .gpio_num   = s_motor_cfg.left_in1_gpio,
            .speed_mode = LEDC_SPEED_MODE,
            .channel    = LEDC_CHANNEL_LEFT_IN1,
            .timer_sel  = LEDC_TIMER_NUM,
            .duty       = 0,
            .hpoint     = 0
        },
        {
            .gpio_num   = s_motor_cfg.left_in2_gpio,
            .speed_mode = LEDC_SPEED_MODE,
            .channel    = LEDC_CHANNEL_LEFT_IN2,
            .timer_sel  = LEDC_TIMER_NUM,
            .duty       = 0,
            .hpoint     = 0
        },
        {
            .gpio_num   = s_motor_cfg.right_in1_gpio,
            .speed_mode = LEDC_SPEED_MODE,
            .channel    = LEDC_CHANNEL_RIGHT_IN1,
            .timer_sel  = LEDC_TIMER_NUM,
            .duty       = 0,
            .hpoint     = 0
        },
        {
            .gpio_num   = s_motor_cfg.right_in2_gpio,
            .speed_mode = LEDC_SPEED_MODE,
            .channel    = LEDC_CHANNEL_RIGHT_IN2,
            .timer_sel  = LEDC_TIMER_NUM,
            .duty       = 0,
            .hpoint     = 0
        }
    };

    for (int i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channels[i]));
    }

    // 初期状態: 全モータ停止（惰性停止）
    motor_stop_all();
}

void motor_set_speed(motor_id_t motor, int16_t speed) {
    // 範囲のクランプ (-255 〜 255)
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    ledc_channel_t ch_in1 = (motor == MOTOR_LEFT) ? LEDC_CHANNEL_LEFT_IN1 : LEDC_CHANNEL_RIGHT_IN1;
    ledc_channel_t ch_in2 = (motor == MOTOR_LEFT) ? LEDC_CHANNEL_LEFT_IN2 : LEDC_CHANNEL_RIGHT_IN2;

    if (speed > 0) {
        // 正転: H(PWM), L
        set_ledc_duty(ch_in1, (uint32_t)speed);
        set_ledc_duty(ch_in2, 0);
    } else if (speed < 0) {
        // 逆転: L, H(PWM)
        set_ledc_duty(ch_in1, 0);
        set_ledc_duty(ch_in2, (uint32_t)(-speed));
    } else {
        // 惰性停止: L, L
        set_ledc_duty(ch_in1, 0);
        set_ledc_duty(ch_in2, 0);
    }
}

void motor_brake(motor_id_t motor) {
    ledc_channel_t ch_in1 = (motor == MOTOR_LEFT) ? LEDC_CHANNEL_LEFT_IN1 : LEDC_CHANNEL_RIGHT_IN1;
    ledc_channel_t ch_in2 = (motor == MOTOR_LEFT) ? LEDC_CHANNEL_LEFT_IN2 : LEDC_CHANNEL_RIGHT_IN2;

    // ブレーキ: H (Duty Max = 255), H (Duty Max = 255)
    set_ledc_duty(ch_in1, 255);
    set_ledc_duty(ch_in2, 255);
}

void motor_stop_all(void) {
    motor_set_speed(MOTOR_LEFT, 0);
    motor_set_speed(MOTOR_RIGHT, 0);
}
