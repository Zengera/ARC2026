#include "MotorController.h"

MotorController::MotorController()
    : _currentLeft(0)
    , _currentRight(0)
{
}

void MotorController::begin() {
    // Arduino-ESP32 3.x の LEDC API を使用
    ledcAttach(PIN_MOTOR_LEFT_A, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(PIN_MOTOR_LEFT_B, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(PIN_MOTOR_RIGHT_A, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(PIN_MOTOR_RIGHT_B, PWM_FREQ, PWM_RESOLUTION);

    // 起動直後はフェイルセーフ停止 (LL)
    stopCoast();
}

void MotorController::setSingleMotor(uint8_t pinA, uint8_t pinB, int16_t speed) {
    if (speed > 0) {
        // 前進: A=PWM, B=LOW
        if (speed > MOTOR_MAX_PWM) speed = MOTOR_MAX_PWM;
        ledcWrite(pinA, speed);
        ledcWrite(pinB, 0);
    } else if (speed < 0) {
        // 後退: A=LOW, B=PWM
        int16_t duty = -speed;
        if (duty > MOTOR_MAX_PWM) duty = MOTOR_MAX_PWM;
        ledcWrite(pinA, 0);
        ledcWrite(pinB, duty);
    } else {
        // 停止: A=LOW, B=LOW (LL: 惰性回転)
        ledcWrite(pinA, 0);
        ledcWrite(pinB, 0);
    }
}

void MotorController::stopCoast() {
    // 惰性回転・停止 (LL)
    ledcWrite(PIN_MOTOR_LEFT_A, 0);
    ledcWrite(PIN_MOTOR_LEFT_B, 0);
    ledcWrite(PIN_MOTOR_RIGHT_A, 0);
    ledcWrite(PIN_MOTOR_RIGHT_B, 0);
    _currentLeft = 0;
    _currentRight = 0;
}

void MotorController::brake() {
    // ショートブレーキ (HH)
    ledcWrite(PIN_MOTOR_LEFT_A, MOTOR_MAX_PWM);
    ledcWrite(PIN_MOTOR_LEFT_B, MOTOR_MAX_PWM);
    ledcWrite(PIN_MOTOR_RIGHT_A, MOTOR_MAX_PWM);
    ledcWrite(PIN_MOTOR_RIGHT_B, MOTOR_MAX_PWM);
    _currentLeft = 0;
    _currentRight = 0;
}

void MotorController::setSpeeds(int16_t left, int16_t right) {
    if (left > MOTOR_MAX_PWM)   left = MOTOR_MAX_PWM;
    if (left < -MOTOR_MAX_PWM)  left = -MOTOR_MAX_PWM;
    if (right > MOTOR_MAX_PWM)  right = MOTOR_MAX_PWM;
    if (right < -MOTOR_MAX_PWM) right = -MOTOR_MAX_PWM;

    setSingleMotor(PIN_MOTOR_LEFT_A, PIN_MOTOR_LEFT_B, left);
    setSingleMotor(PIN_MOTOR_RIGHT_A, PIN_MOTOR_RIGHT_B, right);

    _currentLeft = left;
    _currentRight = right;
}

void MotorController::applyMotionCommand(MotionCommand cmd, int16_t leftSpeed, int16_t rightSpeed) {
    switch (cmd) {
        case MotionCommand::COAST_STOP:
            stopCoast();
            break;
        case MotionCommand::BRAKE:
            brake();
            break;
        case MotionCommand::FORWARD:
            setSpeeds(IR_DRIVE_SPEED, IR_DRIVE_SPEED);
            break;
        case MotionCommand::BACKWARD:
            setSpeeds(-IR_DRIVE_SPEED, -IR_DRIVE_SPEED);
            break;
        case MotionCommand::TURN_LEFT:
            setSpeeds(-IR_DRIVE_SPEED, IR_DRIVE_SPEED);
            break;
        case MotionCommand::TURN_RIGHT:
            setSpeeds(IR_DRIVE_SPEED, -IR_DRIVE_SPEED);
            break;
        case MotionCommand::MIXED_DRIVE:
            setSpeeds(leftSpeed, rightSpeed);
            break;
        default:
            stopCoast();
            break;
    }
}
