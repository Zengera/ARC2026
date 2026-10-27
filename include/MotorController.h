#pragma once

#include <Arduino.h>
#include "Config.h"
#include "Types.h"

class MotorController {
public:
    MotorController();

    void begin();
    void stopCoast();
    void brake();
    void setSpeeds(int16_t left, int16_t right);
    void applyMotionCommand(MotionCommand cmd, int16_t leftSpeed = 0, int16_t rightSpeed = 0);

    int16_t getLeftSpeed() const { return _currentLeft; }
    int16_t getRightSpeed() const { return _currentRight; }

private:
    void setSingleMotor(uint8_t pinA, uint8_t pinB, int16_t speed);

    int16_t _currentLeft;
    int16_t _currentRight;
};
