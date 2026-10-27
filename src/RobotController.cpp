#include "RobotController.h"

RobotController::RobotController()
    : _motor()
    , _ps4()
    , _ir2(PIN_IR_RX_HIGH, "IR2", getPrimaryIRReceiver())
    , _ir4(PIN_IR_RX_LOW, "IR4", getSecondaryIRReceiver())
    , _currentSource(ControlSource::NONE)
    , _currentCommand(MotionCommand::COAST_STOP)
    , _targetLeftSpeed(0)
    , _targetRightSpeed(0)
    , _lastControlTime(0)
    , _lastLogTime(0)
{
}

void RobotController::begin() {
    Serial.begin(SERIAL_BAUD_RATE);

    // モータドライバ初期化 (フェイルセーフ停止LLで開始)
    _motor.begin();

    // 赤外線受信機初期化
    _ir2.begin();
    _ir4.begin();

    // PS4コントローラ初期化 (起動ログとMACアドレス出力を含む)
    _ps4.begin(PS4_BT_MAC);

    _lastControlTime = millis();
    _lastLogTime = millis();
}

void RobotController::update() {
    // 制御周期: 10ms
    unsigned long now = millis();
    if (now - _lastControlTime < CONTROL_CYCLE_MS) {
        return;
    }
    _lastControlTime = now;

    // 実行シーケンス:
    // readInputs() -> determineControlSource() -> generateMotionCommand() -> applyMotorOutput()
    readInputs();
    determineControlSource();
    generateMotionCommand();
    applyMotorOutput();

    // 定期ログ出力 (1秒ごと)
    if (now - _lastLogTime >= LOG_CYCLE_MS) {
        _lastLogTime = now;
        logPeriodic();
    }
}

void RobotController::readInputs() {
    _ir2.update();
    _ir4.update();
    _ps4.update();
}

void RobotController::determineControlSource() {
    // 優先順位: IR2 (GPIO2) -> IR4 (GPIO4) -> PS4
    ControlSource newSource = ControlSource::NONE;

    if (_ir2.inControlWindow()) {
        newSource = ControlSource::IR2;
    } else if (_ir4.inControlWindow()) {
        newSource = ControlSource::IR4;
    } else if (_ps4.isConnected()) {
        newSource = ControlSource::PS4;
    } else {
        newSource = ControlSource::NONE;
    }

    // 制御ソース変更時のログ出力
    if (newSource != _currentSource) {
        _currentSource = newSource;
        switch (_currentSource) {
            case ControlSource::IR2:
                Serial.println("Control Source : IR2");
                break;
            case ControlSource::IR4:
                Serial.println("Control Source : IR4");
                break;
            case ControlSource::PS4:
                Serial.println("Control Source : PS4");
                break;
            case ControlSource::NONE:
                break;
        }
    }
}

void RobotController::generateMotionCommand() {
    _targetLeftSpeed = 0;
    _targetRightSpeed = 0;

    switch (_currentSource) {
        case ControlSource::IR2: {
            _currentCommand = _ir2.getCommand();
            break;
        }

        case ControlSource::IR4: {
            _currentCommand = _ir4.getCommand();
            break;
        }

        case ControlSource::PS4: {
            if (!_ps4.isConnected()) {
                _currentCommand = MotionCommand::COAST_STOP;
                break;
            }

            if (_ps4.isBrakePressed()) {
                _currentCommand = MotionCommand::BRAKE;
                break;
            }

            int16_t throttle = _ps4.getThrottle();
            int16_t steering = _ps4.getSteering();

            if (throttle == 0 && steering == 0) {
                _currentCommand = MotionCommand::COAST_STOP;
            } else {
                _currentCommand = MotionCommand::MIXED_DRIVE;

                // ミキシング計算
                int32_t leftMotor  = static_cast<int32_t>(throttle) + steering;
                int32_t rightMotor = static_cast<int32_t>(throttle) - steering;

                // 比率維持スケーリング (-255〜+255 を超えた場合)
                int32_t maxVal = max(abs(leftMotor), abs(rightMotor));
                if (maxVal > MOTOR_MAX_PWM) {
                    leftMotor  = (leftMotor * MOTOR_MAX_PWM) / maxVal;
                    rightMotor = (rightMotor * MOTOR_MAX_PWM) / maxVal;
                }

                _targetLeftSpeed  = static_cast<int16_t>(leftMotor);
                _targetRightSpeed = static_cast<int16_t>(rightMotor);
            }
            break;
        }

        case ControlSource::NONE:
        default: {
            _currentCommand = MotionCommand::COAST_STOP;
            break;
        }
    }
}

void RobotController::applyMotorOutput() {
    _motor.applyMotionCommand(_currentCommand, _targetLeftSpeed, _targetRightSpeed);
}

void RobotController::logPeriodic() {
    // 1秒ごとの定期ログ
    // IR制御中であってもPS4入力は取得・表示を継続する
    Serial.println("PS4:");
    Serial.printf("LY=%d\n", _ps4.getRawLY());
    Serial.printf("RX=%d\n", _ps4.getRawRX());
    Serial.println();
    Serial.println("Motor:");
    Serial.printf("L=%d\n", _motor.getLeftSpeed());
    Serial.printf("R=%d\n", _motor.getRightSpeed());
}
