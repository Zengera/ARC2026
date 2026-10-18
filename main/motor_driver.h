#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

// 2つのDCモータの定義
typedef enum {
    MOTOR_LEFT = 0,
    MOTOR_RIGHT = 1
} motor_id_t;

// ピン配置の設定構造体
typedef struct {
    gpio_num_t left_in1_gpio;   // 左モータ IN1 (GPIO)
    gpio_num_t left_in2_gpio;   // 左モータ IN2 (GPIO)
    gpio_num_t right_in1_gpio;  // 右モータ IN1 (GPIO)
    gpio_num_t right_in2_gpio;  // 右モータ IN2 (GPIO)
} motor_config_t;

/**
 * @brief モータドライバ（LEDC PWMおよびGPIO）の初期化
 * @param config ピン配置設定（NULLの場合はデフォルトピンを設定）
 */
void motor_driver_init(const motor_config_t *config);

/**
 * @brief モータの速度および回転方向を設定する
 * @param motor 対象モータ (MOTOR_LEFT または MOTOR_RIGHT)
 * @param speed 速度 (-255 〜 255)
 *              正の値: 正転 (IN1 = PWM, IN2 = LOW)
 *              負の値: 逆転 (IN1 = LOW, IN2 = PWM)
 *              0: 惰性停止 (IN1 = LOW, IN2 = LOW)
 */
void motor_set_speed(motor_id_t motor, int16_t speed);

/**
 * @brief モータを電子ブレーキ停止状態にする (IN1 = HIGH, IN2 = HIGH)
 * @param motor 対象モータ (MOTOR_LEFT または MOTOR_RIGHT)
 */
void motor_brake(motor_id_t motor);

/**
 * @brief 全モータを安全停止（惰性停止）させる
 */
void motor_stop_all(void);

#endif // MOTOR_DRIVER_H
