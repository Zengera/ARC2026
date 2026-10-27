#ifndef IRREMOTE_HPP
#define IRREMOTE_HPP

#include <cstdint>
#include <cstring>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief 受信した赤外線データ構造体 (Arduino-IRremote互換)
 */
struct IRData
{
    uint32_t decodedRawData{0}; // 32bit Raw Code (例: 0x00FF18E7)
    uint8_t  address{0};
    uint8_t  command{0};
    bool     isRepeat{false};
};

/**
 * @brief 赤外線受信クラス (Arduino-IRremote互換)
 */
class IRrecv
{
public:
    IRrecv() = default;
    explicit IRrecv(gpio_num_t pin) { begin(pin); }
    ~IRrecv();

    bool begin(gpio_num_t pin);
    bool decode();
    void resume();

    IRData decodedIRData;

private:
    enum class State {
        IDLE,
        HEADER_SPACE,
        DATA_MARK,
        DATA_SPACE
    };

    static void IRAM_ATTR isrHandler(void* arg);
    void handleInterrupt();

    gpio_num_t m_pin{GPIO_NUM_NC};
    QueueHandle_t m_queue{nullptr};

    State    m_state{State::IDLE};
    int64_t  m_lastEdgeTime{0};
    uint32_t m_currentBits{0};
    int      m_bitCount{0};
    uint32_t m_lastValidCode{0};
};

#endif // IRREMOTE_HPP
