#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "CommonTypes.h"
#include <IRremote.hpp>

class IRReceiver
{
public:
    IRReceiver(const char* name, gpio_num_t pin);
    ~IRReceiver() = default;

    bool begin();
    void update();

    /**
     * @brief 最後の有効なIR受信から150ms以内か
     */
    bool isActive() const;

    /**
     * @brief 制御権を保持しているか（受信から 150ms + 500ms = 650ms 以内）
     */
    bool hasControl() const;

    /**
     * @brief 現在のモーションコマンドを取得
     */
    MotionCommand getCommand() const;

    uint32_t getLastCode() const { return m_lastCode; }
    const char* getName() const { return m_name; }

private:
    MotionCommand codeToCommand(uint32_t code) const;

    const char* m_name;
    gpio_num_t m_pin;
    IRrecv m_irRecv;

    uint32_t m_lastCode{0};
    MotionCommand m_currentCmd{MotionCommand::COAST_STOP};
    TickType_t m_lastReceiveTick{0};
    bool m_hasReceivedAny{false};
};

#endif // IR_RECEIVER_H
