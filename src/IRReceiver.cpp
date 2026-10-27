#define SUPPORT_MULTIPLE_RECEIVER_INSTANCES
void UserIRReceiveTimerInterruptHandler();
#include <IRremote.hpp>

#include "IRReceiver.h"

// GPIO4用セカンドレシーバーインスタンス
static IRrecv s_irReceiver4(PIN_IR_RX_LOW);

// 複数レシーバー用タイマー割り込みハンドラ
#if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#endif
void UserIRReceiveTimerInterruptHandler() {
    s_irReceiver4.ReceiveInterruptHandler();
}

IRrecv* getPrimaryIRReceiver() {
    return &IrReceiver;
}

IRrecv* getSecondaryIRReceiver() {
    return &s_irReceiver4;
}

IRReceiver::IRReceiver(uint8_t pin, const char* name, IRrecv* receiver)
    : _pin(pin)
    , _name(name)
    , _receiver(receiver)
    , _lastReceiveTime(0)
    , _lastCommand(MotionCommand::COAST_STOP)
    , _lastRawCode(0)
    , _hasReceivedAny(false)
{
}

void IRReceiver::begin() {
    if (_receiver != nullptr) {
        _receiver->begin(_pin, DISABLE_LED_FEEDBACK);
        Serial.printf("IR Receiver [%s] initialized on GPIO %d\n", _name, _pin);
    } else {
        Serial.printf("IR Receiver [%s] initialization failed (null receiver)\n", _name);
    }
}

MotionCommand IRReceiver::decodeCommand(uint8_t cmd, uint32_t rawData) {
    // 1. コマンド値による判定 (NECプロトコル)
    // 2 : 前進 (0x18)
    // 4 : 左旋回 (0x10)
    // 5 : ショートブレーキ (0x38)
    // 6 : 右旋回 (0x5A)
    // 8 : 後退 (0x4A)
    if (cmd == 0x18) return MotionCommand::FORWARD;
    if (cmd == 0x10) return MotionCommand::TURN_LEFT;
    if (cmd == 0x38) return MotionCommand::BRAKE;
    if (cmd == 0x5A) return MotionCommand::TURN_RIGHT;
    if (cmd == 0x4A) return MotionCommand::BACKWARD;

    // 2. 32bit Rawデータによるフォールバック判定
    uint16_t low16 = rawData & 0xFFFF;
    if (low16 == 0x18E7 || (rawData & 0xFF00) == 0x1800) return MotionCommand::FORWARD;
    if (low16 == 0x10EF || (rawData & 0xFF00) == 0x1000) return MotionCommand::TURN_LEFT;
    if (low16 == 0x38C7 || (rawData & 0xFF00) == 0x3800) return MotionCommand::BRAKE;
    if (low16 == 0x5AA5 || (rawData & 0xFF00) == 0x5A00) return MotionCommand::TURN_RIGHT;
    if (low16 == 0x4AB5 || (rawData & 0xFF00) == 0x4A00) return MotionCommand::BACKWARD;

    // 不正コマンド / 未定義コード
    return MotionCommand::COAST_STOP;
}

void IRReceiver::update() {
    if (_receiver == nullptr) return;

    if (_receiver->decode()) {
        uint32_t raw = _receiver->decodedIRData.decodedRawData;
        bool isRepeat = (_receiver->decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT);

        if (!isRepeat && raw != 0) {
            _lastRawCode = raw;
            // 受信コードを16進数で表示 (例: IR2 : 0x00FF18E7)
            Serial.printf("%s : 0x%08X\n", _name, (unsigned int)_lastRawCode);

            // コマンド解析
            MotionCommand cmd = decodeCommand(_receiver->decodedIRData.command, raw);
            _lastCommand = cmd;
        } else if (isRepeat) {
            // 長押しリピート中: 前回のコマンドを維持
        }

        _lastReceiveTime = millis();
        _hasReceivedAny = true;

        _receiver->resume(); // 次の信号受信を有効化
    }
}

bool IRReceiver::hasActiveSignal() const {
    if (!_hasReceivedAny) return false;
    return (millis() - _lastReceiveTime) <= IR_SIGNAL_TIMEOUT_MS;
}

bool IRReceiver::inControlWindow() const {
    if (!_hasReceivedAny) return false;
    return (millis() - _lastReceiveTime) <= (IR_SIGNAL_TIMEOUT_MS + IR_RELEASE_DELAY_MS);
}

MotionCommand IRReceiver::getCommand() const {
    if (hasActiveSignal()) {
        return _lastCommand;
    }
    // タイムアウトまたは信号途絶中は停止
    return MotionCommand::COAST_STOP;
}

