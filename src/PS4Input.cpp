#include "PS4Input.h"
#include <esp_mac.h>

static PS4Input* s_instance = nullptr;

PS4Input::PS4Input()
    : _lastConnected(false)
{
    s_instance = this;
}

void PS4Input::onConnect() {
    Serial.println("PS4 Connected");
}

void PS4Input::onDisconnect() {
    Serial.println("PS4 Disconnected");
}

void PS4Input::begin(const char* mac) {
    // コールバック登録
    PS4.attachOnConnect(onConnect);
    PS4.attachOnDisconnect(onDisconnect);

    // PS4Controllerの初期化
    bool success = false;
    if (mac != nullptr && strlen(mac) > 0) {
        success = PS4.begin(mac);
    } else {
        success = PS4.begin();
    }

    // 起動時にESP32のBluetooth MACアドレスをシリアル出力
    uint8_t btMac[6];
    esp_read_mac(btMac, ESP_MAC_BT);

    Serial.println("System Start");
    Serial.println("ESP32 Bluetooth MAC:");
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             btMac[0], btMac[1], btMac[2], btMac[3], btMac[4], btMac[5]);
    Serial.println(macStr);

    if (success) {
        Serial.println("PS4 Controller Host initialized. Waiting for controller...");
    } else {
        Serial.println("PS4 Controller Host initialization failed.");
    }
}

int16_t PS4Input::applyDeadzoneAndScale(int8_t rawVal) {
    if (rawVal > -PS4_DEADZONE && rawVal < PS4_DEADZONE) {
        return 0;
    }

    // デッドゾーン外の値を -255 〜 255 へスケーリング
    // 生値 -128〜127 を 2倍 (-256〜254) してクランプ
    int16_t scaled = static_cast<int16_t>(rawVal) * 2;
    if (scaled > MOTOR_MAX_PWM)  scaled = MOTOR_MAX_PWM;
    if (scaled < -MOTOR_MAX_PWM) scaled = -MOTOR_MAX_PWM;
    return scaled;
}

void PS4Input::update() {
    bool currentConnected = PS4.isConnected();

    // 接続状態遷移チェック (コールバックと併せて補完)
    if (currentConnected && !_lastConnected) {
        // 接続直後
    } else if (!currentConnected && _lastConnected) {
        // 切断直後
    }
    _lastConnected = currentConnected;

    _data.connected = currentConnected;

    if (currentConnected) {
        _data.rawLY = PS4.LStickY();
        _data.rawRX = PS4.RStickX();
        _data.throttle = applyDeadzoneAndScale(_data.rawLY);
        _data.steering = applyDeadzoneAndScale(_data.rawRX);
        _data.brakeButton = PS4.R1();
    } else {
        _data.rawLY = 0;
        _data.rawRX = 0;
        _data.throttle = 0;
        _data.steering = 0;
        _data.brakeButton = false;
    }
}
