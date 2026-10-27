#ifndef ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_H

#include "CommonTypes.h"
#include "MotorController.h"
#include "PS4Input.h"
#include "IRReceiver.h"

class RobotController
{
public:
    static constexpr gpio_num_t PIN_IR_HIGH = GPIO_NUM_2;
    static constexpr gpio_num_t PIN_IR_LOW  = GPIO_NUM_4;

    RobotController();
    ~RobotController() = default;

    void begin();
    void update();

private:
    void readInputs();
    void determineControlSource();
    void generateMotionCommand();
    void applyMotorOutput();
    void printPeriodicLog();

    MotorController m_motorController;
    PS4Input m_ps4Input;
    IRReceiver m_irReceiver2;
    IRReceiver m_irReceiver4;

    ControlSource m_currentSource{ControlSource::NONE};
    MotionCommand m_currentCommand{MotionCommand::COAST_STOP};

    int16_t m_targetLeftSpeed{0};
    int16_t m_targetRightSpeed{0};

    TickType_t m_lastLogTick{0};
};

#endif // ROBOT_CONTROLLER_H
