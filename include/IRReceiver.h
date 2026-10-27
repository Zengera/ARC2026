#pragma once

#include <Arduino.h>
#include "Config.h"
#include "Types.h"

// IRrecvの前方宣言 (IRremote.hppは実装を含むためcppファイルでのみインクルードする)
class IRrecv;

class IRReceiver {
public:
    IRReceiver(uint8_t pin, const char* name, IRrecv* receiver);

    void begin();
    void update();

    bool hasActiveSignal() const;
    bool inControlWindow() const;
    MotionCommand getCommand() const;
    uint32_t getLastRawCode() const { return _lastRawCode; }
    const char* getName() const { return _name; }

private:
    MotionCommand decodeCommand(uint8_t cmd, uint32_t rawData);

    uint8_t       _pin;
    const char*   _name;
    IRrecv*       _receiver;
    unsigned long _lastReceiveTime;
    MotionCommand _lastCommand;
    uint32_t      _lastRawCode;
    bool          _hasReceivedAny;
};

// レシーバーインスタンスへのアクセサ
IRrecv* getPrimaryIRReceiver();
IRrecv* getSecondaryIRReceiver();

