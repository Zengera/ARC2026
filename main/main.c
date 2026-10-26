#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "ps4.h"
#include "motor_driver.h"

static const char *TAG = "PS4_MAIN";

// ESP32のBT MACアドレス: e0:8c:fe:fe:a6:2e
static const uint8_t ps4_bt_mac[6] = {0xe0, 0x8c, 0xfe, 0xfe, 0xa6, 0x2e};

static uint8_t current_r = 0, current_g = 0, current_b = 0;
static ps4_t latest_ps4_data;
static volatile bool s_is_connected = false;
static volatile TickType_t s_last_packet_tick = 0;
static int16_t s_current_left_speed = 0;
static int16_t s_current_right_speed = 0;

// スティックの不感帯（遊びの微小なブレでモータが回らないようにする）
#define STICK_DEADZONE 15

/**
 * @brief スティック値にデッドゾーンを適用
 */
static inline int16_t apply_deadzone(int8_t val)
{
    if (val > -STICK_DEADZONE && val < STICK_DEADZONE) {
        return 0;
    }
    return val;
}

/**
 * @brief PS4コントローラの入力に基づいてモータ速度を更新する
 * 
 * 操作方法:
 *   - 左スティック縦 (LY): 前進 (+) / 後退 (-)
 *   - 右スティック横 (RX): 右旋回 (+) / 左旋回 (-)
 *   - [O] Circle ボタン押下中: 急ブレーキ (電子ブレーキ)
 */
static void update_motor_control(ps4_t ps4)
{
    // [O] ボタンが押されている時はブレーキ
    if (ps4.button.circle) {
        motor_brake(MOTOR_LEFT);
        motor_brake(MOTOR_RIGHT);
        s_current_left_speed = 0;
        s_current_right_speed = 0;
        return;
    }

    // スティック値の取得 (LY: 前後, RX: 旋回)
    int16_t throttle = apply_deadzone(ps4.analog.stick.ly);
    int16_t steer    = apply_deadzone(ps4.analog.stick.rx);

    // スティック範囲 (-128〜127) を モータ速度範囲 (-255〜255) にスケーリング
    // 左モータ = 前進 + 旋回
    // 右モータ = 前進 - 旋回
    int16_t left_speed  = (throttle + steer) * 2;
    int16_t right_speed = (throttle - steer) * 2;

    // クランプ処理 (-255 〜 255)
    if (left_speed > 255)   left_speed = 255;
    if (left_speed < -255)  left_speed = -255;
    if (right_speed > 255)  right_speed = 255;
    if (right_speed < -255) right_speed = -255;

    motor_set_speed(MOTOR_LEFT, left_speed);
    motor_set_speed(MOTOR_RIGHT, right_speed);

    s_current_left_speed = left_speed;
    s_current_right_speed = right_speed;
}

/**
 * @brief 重複送信を防ぐLED更新関数
 */
static void set_led_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (current_r == r && current_g == g && current_b == b) {
        return; // 同じ色の場合は送信しない
    }
    current_r = r;
    current_g = g;
    current_b = b;
    ps4SetLed(r, g, b);
}

/**
 * @brief PS4コントローラの接続・切断コールバック
 */
static void on_ps4_connection(uint8_t is_connected)
{
    s_is_connected = is_connected ? true : false;

    if (is_connected) {
        ESP_LOGI(TAG, "PS4 Controller CONNECTED!");
        // 接続通知としてLEDを青に点灯 (キャッシュをリセットして確実に送信)
        current_r = current_g = current_b = 255;
        set_led_color(0, 0, 255);
        s_last_packet_tick = xTaskGetTickCount();
    } else {
        ESP_LOGW(TAG, "PS4 Controller DISCONNECTED! Stopping motors.");
    }
}

/**
 * @brief PS4コントローラのデータ受信イベントコールバック (Bluetoothタスクで動作)
 * @note  Cache errorやブロックを防ぐため、ハードウェア制御は直接行わず、
 *        最新データを保存して別タスク(motor_task)でモータを駆動します。
 */
static void on_ps4_event(ps4_t ps4, ps4_event_t event)
{
    latest_ps4_data = ps4;
    s_last_packet_tick = xTaskGetTickCount();

    // ボタン押下イベントの検出例
    if (event.button_down.cross) {
        ESP_LOGI(TAG, "Button pressed: [X] Cross");
        set_led_color(255, 0, 0); // 赤
    }
    if (event.button_down.circle) {
        ESP_LOGI(TAG, "Button pressed: [O] Circle (Brake)");
        set_led_color(255, 0, 255); // マゼンタ
    }
    if (event.button_down.triangle) {
        ESP_LOGI(TAG, "Button pressed: [/\\\\] Triangle");
        set_led_color(0, 255, 0); // 緑
    }
    if (event.button_down.square) {
        ESP_LOGI(TAG, "Button pressed: [[]] Square");
        set_led_color(255, 255, 0); // 黄
    }

    if (event.button_down.up)    ESP_LOGI(TAG, "Button pressed: UP");
    if (event.button_down.down)  ESP_LOGI(TAG, "Button pressed: DOWN");
    if (event.button_down.left)  ESP_LOGI(TAG, "Button pressed: LEFT");
    if (event.button_down.right) ESP_LOGI(TAG, "Button pressed: RIGHT");

    if (event.button_down.l1) ESP_LOGI(TAG, "Button pressed: L1");
    if (event.button_down.r1) ESP_LOGI(TAG, "Button pressed: R1");
    if (event.button_down.l2) ESP_LOGI(TAG, "Button pressed: L2 (Analog: %d)", ps4.analog.button.l2);
    if (event.button_down.r2) ESP_LOGI(TAG, "Button pressed: R2 (Analog: %d)", ps4.analog.button.r2);

    if (event.button_down.share)   ESP_LOGI(TAG, "Button pressed: SHARE");
    if (event.button_down.options) ESP_LOGI(TAG, "Button pressed: OPTIONS");
    if (event.button_down.ps)      ESP_LOGI(TAG, "Button pressed: PS");
    if (event.button_down.touchpad)ESP_LOGI(TAG, "Button pressed: TOUCHPAD");

    // ボタン離しイベント (記号ボタンがすべて離されたら青に戻す)
    if (event.button_up.cross || event.button_up.circle ||
        event.button_up.triangle || event.button_up.square) {
        if (!ps4.button.cross && !ps4.button.circle &&
            !ps4.button.triangle && !ps4.button.square) {
            set_led_color(0, 0, 255);
        }
    }
}

/**
 * @brief 独立したモータ制御タスク (50Hz / 20ms周期)
 * 
 * Bluetoothの内部スタックタスクからモータPWM制御を分離することで、
 * Flashアクセス中のCache errorを防止し、通信途絶時の自動停止を保証します。
 */
static void motor_task(void *pvParameters)
{
    bool is_stopped = true;

    while (1) {
        TickType_t now = xTaskGetTickCount();

        // 接続中 かつ 直近300ms以内にパケットを受信している場合のみモータを駆動
        if (s_is_connected && (now - s_last_packet_tick < pdMS_TO_TICKS(300))) {
            update_motor_control(latest_ps4_data);
            is_stopped = false;
        } else {
            // 切断時または通信途絶時は安全のため停止 (すでに停止中なら重複実行しない)
            if (!is_stopped) {
                motor_stop_all();
                s_current_left_speed = 0;
                s_current_right_speed = 0;
                is_stopped = true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 20ms周期 (50Hz)
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing PS4 ESP32 Motor Controller Application...");

    // 1. モータドライバの初期化 (デフォルトピン: Left=GPIO18,19 / Right=GPIO22,23)
    //    ピンを変更する場合は motor_config_t 構造体にピン番号を設定して渡してください
    motor_driver_init(NULL);

    // 2. 独立したモータ制御タスクの起動 (Bluetoothスタックから分離して50Hzで安全駆動)
    xTaskCreate(motor_task, "motor_task", 4096, NULL, 5, NULL);

    // 3. NVSの初期化 (Bluetoothスタックに必須)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 4. 指定されたBT MACアドレス (e0:8c:fe:fe:a6:2e) を設定
    ps4SetBluetoothMacAddress(ps4_bt_mac);

    // 設定後のBT MACアドレスを確認出力
    uint8_t current_mac[6];
    esp_read_mac(current_mac, ESP_MAC_BT);
    ESP_LOGI(TAG, "Configured Bluetooth MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             current_mac[0], current_mac[1], current_mac[2],
             current_mac[3], current_mac[4], current_mac[5]);

    // 5. コールバックの設定
    ps4SetConnectionCallback(on_ps4_connection);
    ps4SetEventCallback(on_ps4_event);

    // 6. PS4ライブラリの初期化 (SPPおよびL2CAPサーバー起動)
    ps4Init();

    ESP_LOGI(TAG, "Waiting for PS4 Controller connection (Press PS button)...");

    // 7. メインループ: 接続中の場合、定期的にバッテリー残量やモータ速度を出力
    while (1) {
        if (ps4IsConnected()) {
            ESP_LOGI(TAG, "Battery: %d%% | Motor: [L: %4d, R: %4d] | Stick: [LY: %4d, RX: %4d]",
                     latest_ps4_data.status.battery * 10,
                     s_current_left_speed, s_current_right_speed,
                     latest_ps4_data.analog.stick.ly, latest_ps4_data.analog.stick.rx);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
