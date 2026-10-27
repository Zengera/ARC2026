#ifndef PS4_INPUT_H
#define PS4_INPUT_H

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
extern "C" {
#include "ps4.h"
}

class PS4Input
{
public:
    PS4Input();
    ~PS4Input() = default;

    /**
     * @brief NVSおよびBluetooth/PS4スタックの初期化
     */
    bool begin();

    /**
     * @brief 毎周期の入力更新（接続タイムアウト等のチェック）
     */
    void update();

    bool isConnected() const { return m_connected; }
    int getThrottle() const { return m_throttle; }
    int getSteering() const { return m_steering; }
    bool isBrakeButtonPressed() const { return m_brakeButton; }

    int8_t getRawLY() const { return m_rawLY; }
    int8_t getRawRX() const { return m_rawRX; }

    /**
     * @brief スティック入力からミキシングと比率維持スケーリングを適用した左右速度を算出
     */
    void getMixedSpeeds(int16_t &leftMotor, int16_t &rightMotor) const;

    static void onConnectionStatic(uint8_t is_connected);
    static void onEventStatic(ps4_t ps4, ps4_event_t event);

private:
    static PS4Input* s_instance;

    void handleConnection(bool is_connected);
    void handleEvent(const ps4_t &ps4, const ps4_event_t &event);

    static int16_t applyDeadzone(int8_t val);

    volatile bool m_connected{false};
    volatile TickType_t m_lastPacketTick{0};

    int m_throttle{0};
    int m_steering{0};
    bool m_brakeButton{false};

    int8_t m_rawLY{0};
    int8_t m_rawRX{0};
};

#endif // PS4_INPUT_H
