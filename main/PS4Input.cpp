#include "PS4Input.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"

static const uint8_t s_ps4_bt_mac[6] = {0xe0, 0x8c, 0xfe, 0xfe, 0xa6, 0x2e};

PS4Input* PS4Input::s_instance = nullptr;

PS4Input::PS4Input()
{
    s_instance = this;
}

int16_t PS4Input::applyDeadzone(int8_t val)
{
    // デッドゾーン: ±15
    if (val > -15 && val < 15) {
        return 0;
    }
    // -128〜127 を -255〜255 にスケール
    int32_t scaled = static_cast<int32_t>(val) * 2;
    return static_cast<int16_t>(std::clamp<int32_t>(scaled, -255, 255));
}

bool PS4Input::begin()
{
    printf("System Start\n");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ps4SetBluetoothMacAddress(s_ps4_bt_mac);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    printf("ESP32 Bluetooth MAC:\n%02X:%02X:%02X:%02X:%02X:%02X\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ps4SetConnectionCallback(onConnectionStatic);
    ps4SetEventCallback(onEventStatic);

    ps4Init();
    return true;
}

void PS4Input::onConnectionStatic(uint8_t is_connected)
{
    if (s_instance) {
        s_instance->handleConnection(is_connected != 0);
    }
}

void PS4Input::handleConnection(bool is_connected)
{
    if (m_connected == is_connected) {
        return;
    }
    m_connected = is_connected;
    if (m_connected) {
        printf("PS4 Connected\n");
        m_lastPacketTick = xTaskGetTickCount();
        ps4SetLed(0, 0, 255);
    } else {
        printf("PS4 Disconnected\n");
        m_throttle = 0;
        m_steering = 0;
        m_brakeButton = false;
        m_rawLY = 0;
        m_rawRX = 0;
    }
}

void PS4Input::onEventStatic(ps4_t ps4, ps4_event_t event)
{
    if (s_instance) {
        s_instance->handleEvent(ps4, event);
    }
}

void PS4Input::handleEvent(const ps4_t &ps4, const ps4_event_t &event)
{
    m_lastPacketTick = xTaskGetTickCount();

    if (!m_connected) {
        handleConnection(true);
    }

    m_rawLY = ps4.analog.stick.ly;
    m_rawRX = ps4.analog.stick.rx;

    m_throttle = applyDeadzone(m_rawLY);
    m_steering = applyDeadzone(m_rawRX);

    // ショートブレーキ入力: PS4 R1ボタン
    m_brakeButton = (ps4.button.r1 != 0);
}

void PS4Input::update()
{
    if (m_connected) {
        TickType_t now = xTaskGetTickCount();
        if ((now - m_lastPacketTick) > pdMS_TO_TICKS(500)) {
            // パケット途絶タイムアウト
            handleConnection(false);

            // Bluetoothスタックの接続状態をリセットし、再接続を受け入れられるよう待受を再初期化
            ps4Disconnect();
        }
    }
}

void PS4Input::getMixedSpeeds(int16_t &leftMotor, int16_t &rightMotor) const
{
    int32_t left  = m_throttle + m_steering;
    int32_t right = m_throttle - m_steering;

    int32_t maxVal = std::max(std::abs(left), std::abs(right));
    if (maxVal > 255) {
        left  = (left * 255) / maxVal;
        right = (right * 255) / maxVal;
    }

    leftMotor  = static_cast<int16_t>(left);
    rightMotor = static_cast<int16_t>(right);
}
