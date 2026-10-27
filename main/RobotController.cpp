#include "RobotController.h"
#include <cstdio>
#include "esp_log.h"

static const char *TAG = "RobotController";

RobotController::RobotController()
    : m_irReceiver2("IR2", PIN_IR_HIGH),
      m_irReceiver4("IR4", PIN_IR_LOW)
{
}

void RobotController::begin()
{
    ESP_LOGI(TAG, "Initializing RobotController components...");

    m_motorController.begin();
    m_motorController.coastStop();

    m_irReceiver2.begin();
    m_irReceiver4.begin();

    m_ps4Input.begin();

    m_lastLogTick = xTaskGetTickCount();
    ESP_LOGI(TAG, "RobotController initialized.");
}

void RobotController::readInputs()
{
    m_irReceiver2.update();
    m_irReceiver4.update();
    m_ps4Input.update();
}

void RobotController::determineControlSource()
{
    ControlSource newSource = ControlSource::NONE;

    // 優先順位: IR2 (最高) -> IR4 -> PS4
    if (m_irReceiver2.hasControl()) {
        newSource = ControlSource::IR2;
    } else if (m_irReceiver4.hasControl()) {
        newSource = ControlSource::IR4;
    } else if (m_ps4Input.isConnected()) {
        newSource = ControlSource::PS4;
    } else {
        newSource = ControlSource::NONE;
    }

    if (newSource != m_currentSource) {
        m_currentSource = newSource;
        switch (m_currentSource) {
        case ControlSource::IR2:
            printf("Control Source : IR2\n");
            break;
        case ControlSource::IR4:
            printf("Control Source : IR4\n");
            break;
        case ControlSource::PS4:
            printf("Control Source : PS4\n");
            break;
        case ControlSource::NONE:
        default:
            break;
        }
    }
}

void RobotController::generateMotionCommand()
{
    m_targetLeftSpeed = 0;
    m_targetRightSpeed = 0;

    switch (m_currentSource) {
    case ControlSource::IR2:
        m_currentCommand = m_irReceiver2.getCommand();
        break;

    case ControlSource::IR4:
        m_currentCommand = m_irReceiver4.getCommand();
        break;

    case ControlSource::PS4:
        if (!m_ps4Input.isConnected()) {
            m_currentCommand = MotionCommand::COAST_STOP;
        } else if (m_ps4Input.isBrakeButtonPressed()) {
            m_currentCommand = MotionCommand::BRAKE;
        } else {
            m_currentCommand = MotionCommand::MIXED_DRIVE;
            m_ps4Input.getMixedSpeeds(m_targetLeftSpeed, m_targetRightSpeed);
            if (m_targetLeftSpeed == 0 && m_targetRightSpeed == 0) {
                m_currentCommand = MotionCommand::COAST_STOP;
            }
        }
        break;

    case ControlSource::NONE:
    default:
        // フェイルセーフ: 未接続/タイムアウト時は停止 (LL)
        m_currentCommand = MotionCommand::COAST_STOP;
        break;
    }
}

void RobotController::applyMotorOutput()
{
    m_motorController.executeCommand(m_currentCommand, m_targetLeftSpeed, m_targetRightSpeed);
}

void RobotController::printPeriodicLog()
{
    TickType_t now = xTaskGetTickCount();
    // 1秒ごと (1000ms)
    if ((now - m_lastLogTick) >= pdMS_TO_TICKS(1000)) {
        m_lastLogTick = now;

        // IR制御中であってもPS4入力は取得・表示を継続する
        printf("PS4: LY=%d,RX=%d\nMotor: L=%d,R=%d\n",
               m_ps4Input.getRawLY(),
               m_ps4Input.getRawRX(),
               m_motorController.getLeftSpeed(),
               m_motorController.getRightSpeed());
    }
}

void RobotController::update()
{
    readInputs();
    determineControlSource();
    generateMotionCommand();
    applyMotorOutput();
    printPeriodicLog();
}
