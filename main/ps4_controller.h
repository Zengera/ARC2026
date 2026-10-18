#ifndef PS4_CONTROLLER_H
#define PS4_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// デフォルトのBT MACアドレス (仮設定: 00:0A:0A:0A:0A:0A)
#define PS4_DEFAULT_MAC_ADDR_STR "00:0A:0A:0A:0A:0A"

// PS4コントローラのステータス構造体
typedef struct {
    uint8_t left_stick_x;  // 0 〜 255 (ニュートラル: 128)
    uint8_t left_stick_y;  // 0 〜 255 (上: 0, 下: 255, ニュートラル: 128)
    uint8_t right_stick_x; // 0 〜 255 (左: 0, 右: 255, ニュートラル: 128)
    uint8_t right_stick_y; // 0 〜 255 (上: 0, 下: 255, ニュートラル: 128)
    bool is_connected;     // 接続状態
} ps4_controller_state_t;

/**
 * @brief PS4コントローラとのBluetooth通信処理を初期化・開始する
 * @param mac_str ターゲットのBT MACアドレス文字列 (例: "00:0A:0A:0A:0A:0A")。NULL時はデフォルト使用。
 * @return esp_err_t ESP_OKで成功
 */
esp_err_t ps4_controller_init(const char *mac_str);

/**
 * @brief 現在のコントローラのステータスを取得する
 * @param state ステータス格納用ポインタ
 */
void ps4_controller_get_state(ps4_controller_state_t *state);

/**
 * @brief コントローラが接続されているか確認する
 * @return true 接続中 / false 切断中
 */
bool ps4_controller_is_connected(void);

#endif // PS4_CONTROLLER_H
