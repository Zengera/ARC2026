#pragma once

#include <Arduino.h>
#include "Config.h"
#include "Types.h"
#include "MotorController.h"
#include "PS4Input.h"
#include "IRReceiver.h"

class RobotController {
public:
    RobotController();

    void begin();
    void update();

private:
    void readInputs();
    void determineControlSource();
    void generateMotionCommand();
    void applyMotorOutput();
    void logPeriodic();

    MotorController _motor;
    PS4Input        _ps4;
    IRReceiver      _ir2;
    IRReceiver      _ir4;

    ControlSource   _currentSource;
    MotionCommand   _currentCommand;
    int16_t         _targetLeftSpeed;
    int16_t         _targetRightSpeed;

    unsigned long   _lastControlTime;
    unsigned long   _lastLogTime;
};
