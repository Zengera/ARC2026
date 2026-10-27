#include "IRremote.hpp"
#include <cstdio>
#include "esp_log.h"

static const char *TAG = "IRremote";
static bool s_isrServiceInstalled = false;

IRrecv::~IRrecv()
{
    if (m_pin != GPIO_NUM_NC) {
        gpio_isr_handler_remove(m_pin);
    }
    if (m_queue) {
        vQueueDelete(m_queue);
        m_queue = nullptr;
    }
}

void IRAM_ATTR IRrecv::isrHandler(void* arg)
{
    IRrecv* receiver = static_cast<IRrecv*>(arg);
    receiver->handleInterrupt();
}

void IRAM_ATTR IRrecv::handleInterrupt()
{
    int level = gpio_get_level(m_pin);
    int64_t now = esp_timer_get_time();
    int64_t duration = now - m_lastEdgeTime;
    m_lastEdgeTime = now;

    if (level == 1) {
        // ピンがHIGHになった（直前のLOWパルス=マークが終了）
        if (m_state == State::IDLE) {
            // リーダーの9000us LOW判定 (7000〜11000us)
            if (duration >= 7000 && duration <= 11500) {
                m_state = State::HEADER_SPACE;
            }
        } else if (m_state == State::DATA_MARK) {
            // 各ビットの560us LOW判定 (300〜950us)
            if (duration >= 300 && duration <= 950) {
                m_state = State::DATA_SPACE;
            } else {
                m_state = State::IDLE;
            }
        }
    } else {
        // ピンがLOWになった（直前のHIGHパルス=スペースが終了）
        if (m_state == State::HEADER_SPACE) {
            if (duration >= 3500 && duration <= 5500) {
                // 通常データフレームの4500us HIGH
                m_state = State::DATA_MARK;
                m_currentBits = 0;
                m_bitCount = 0;
            } else if (duration >= 1500 && duration <= 3200) {
                // リピートフレームの2250us HIGH
                m_state = State::IDLE;
                IRData data;
                data.isRepeat = true;
                data.decodedRawData = m_lastValidCode;
                data.address = static_cast<uint8_t>((m_lastValidCode >> 24) & 0xFF);
                data.command = static_cast<uint8_t>((m_lastValidCode >> 8) & 0xFF);
                BaseType_t high_task_wakeup = pdFALSE;
                xQueueSendFromISR(m_queue, &data, &high_task_wakeup);
            } else {
                m_state = State::IDLE;
            }
        } else if (m_state == State::DATA_SPACE) {
            bool bitVal = false;
            bool validBit = false;

            if (duration >= 1200 && duration <= 2200) {
                // ビット1 (1690us HIGH)
                bitVal = true;
                validBit = true;
            } else if (duration >= 300 && duration <= 900) {
                // ビット0 (560us HIGH)
                bitVal = false;
                validBit = true;
            }

            if (validBit) {
                int byteIdx = m_bitCount / 8;
                int bitIdx  = m_bitCount % 8;
                if (bitVal) {
                    m_currentBits |= (1UL << (byteIdx * 8 + bitIdx));
                }
                m_bitCount++;

                if (m_bitCount >= 32) {
                    // 32ビット受信完了
                    uint8_t b0 = static_cast<uint8_t>((m_currentBits >> 0)  & 0xFF);
                    uint8_t b1 = static_cast<uint8_t>((m_currentBits >> 8)  & 0xFF);
                    uint8_t b2 = static_cast<uint8_t>((m_currentBits >> 16) & 0xFF);
                    uint8_t b3 = static_cast<uint8_t>((m_currentBits >> 24) & 0xFF);

                    uint32_t raw = (static_cast<uint32_t>(b0) << 24) |
                                   (static_cast<uint32_t>(b1) << 16) |
                                   (static_cast<uint32_t>(b2) << 8)  |
                                   static_cast<uint32_t>(b3);

                    m_lastValidCode = raw;

                    IRData data;
                    data.isRepeat = false;
                    data.decodedRawData = raw;
                    data.address = b0;
                    data.command = b2;

                    BaseType_t high_task_wakeup = pdFALSE;
                    xQueueSendFromISR(m_queue, &data, &high_task_wakeup);
                    m_state = State::IDLE;
                } else {
                    m_state = State::DATA_MARK;
                }
            } else {
                m_state = State::IDLE;
            }
        }
    }
}

bool IRrecv::begin(gpio_num_t pin)
{
    m_pin = pin;
    m_queue = xQueueCreate(4, sizeof(IRData));
    if (!m_queue) {
        ESP_LOGE(TAG, "Failed to create IR queue");
        return false;
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = (1ULL << m_pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    if (!s_isrServiceInstalled) {
        esp_err_t err = gpio_install_isr_service(0);
        if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
            s_isrServiceInstalled = true;
        } else {
            ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(err));
            return false;
        }
    }

    ESP_ERROR_CHECK(gpio_isr_handler_add(m_pin, isrHandler, this));
    ESP_LOGI(TAG, "IRrecv started on GPIO %d", m_pin);
    return true;
}

bool IRrecv::decode()
{
    if (!m_queue) return false;
    return (xQueueReceive(m_queue, &decodedIRData, 0) == pdPASS);
}

void IRrecv::resume()
{
    // decode() 後の次のデータ受信準備（本実装では割り込み側でキューイングされるため常時稼働）
}
