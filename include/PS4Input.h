#pragma once

#include <Arduino.h>
#include <PS4Controller.h>
#include "Config.h"
#include "Types.h"

class PS4Input {
public:
    PS4Input();

    void begin(const char* mac = nullptr);
    void update();

    const PS4InputData& getData() const { return _data; }
    bool isConnected() const { return _data.connected; }
    int16_t getThrottle() const { return _data.throttle; }
    int16_t getSteering() const { return _data.steering; }
    int8_t getRawLY() const { return _data.rawLY; }
    int8_t getRawRX() const { return _data.rawRX; }
    bool isBrakePressed() const { return _data.brakeButton; }

private:
    static void onConnect();
    static void onDisconnect();

    int16_t applyDeadzoneAndScale(int8_t rawVal);

    PS4InputData _data;
    bool _lastConnected;
};
