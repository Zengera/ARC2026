#include "IRReceiver.h"
#include <cstdio>
#include "esp_log.h"

static const char *TAG = "IRReceiver";

IRReceiver::IRReceiver(const char* name, gpio_num_t pin)
    : m_name(name), m_pin(pin)
{
}

bool IRReceiver::begin()
{
    ESP_LOGI(TAG, "[%s] Initializing IRReceiver on GPIO %d", m_name, m_pin);
    return m_irRecv.begin(m_pin);
}

MotionCommand IRReceiver::codeToCommand(uint32_t code) const
{
    switch (code) {
    // 2 : 前進 (0x02FD1FE0, 0x02FD0FF0)
    case 0x02FD1FE0:
    case 0x02FD0FF0:
    case 0x00FF18E7: // 標準NEC例
        return MotionCommand::FORWARD;

    // 4 : 左旋回 (0x02FD21DE, 0x02FD11EE)
    case 0x02FD21DE:
    case 0x02FD11EE:
    case 0x00FF08F7: // 標準NEC例
        return MotionCommand::TURN_LEFT;

    // 6 : 右旋回 (0x02FD23DC, 0x02FD13EC)
    case 0x02FD23DC:
    case 0x02FD13EC:
    case 0x00FF5AA5: // 標準NEC例
        return MotionCommand::TURN_RIGHT;

    // 8 : 後退 (0x02FD25DA, 0x02FD15EA)
    case 0x02FD25DA:
    case 0x02FD15EA:
    case 0x00FF52AD: // 標準NEC例
        return MotionCommand::BACKWARD;

    // 5 : 停止 (0x02FD22DD, 0x02FD12ED, 0x02FD00FF)
    case 0x02FD22DD:
    case 0x02FD12ED:
    case 0x02FD00FF:
    case 0x00FF1CE3: // 標準NEC例
        return MotionCommand::BRAKE;

    default:
        // 不正コマンド / 未定義コード: 停止 (フェイルセーフ)
        return MotionCommand::COAST_STOP;
    }
}

void IRReceiver::update()
{
    while (m_irRecv.decode()) {
        m_lastReceiveTick = xTaskGetTickCount();
        m_hasReceivedAny = true;

        if (m_irRecv.decodedIRData.isRepeat) {
            // 長押し中は動作継続 (直前のコマンドを維持)
        } else {
            m_lastCode = m_irRecv.decodedIRData.decodedRawData;
            m_currentCmd = codeToCommand(m_lastCode);
            // 仕様: 受信コードを16進数で表示 (例: "IR2 : 0x00FF18E7")
            printf("%s : 0x%08lX\n", m_name, static_cast<unsigned long>(m_lastCode));
        }

        m_irRecv.resume();
    }
}

bool IRReceiver::isActive() const
{
    if (!m_hasReceivedAny) return false;
    // 150ms信号を受信しなければ停止
    return (xTaskGetTickCount() - m_lastReceiveTick) <= pdMS_TO_TICKS(150);
}

bool IRReceiver::hasControl() const
{
    if (!m_hasReceivedAny) return false;
    // 150msタイムアウト + 500ms復帰待機 = 650ms
    return (xTaskGetTickCount() - m_lastReceiveTick) <= pdMS_TO_TICKS(650);
}

MotionCommand IRReceiver::getCommand() const
{
    return isActive() ? m_currentCmd : MotionCommand::COAST_STOP;
}
