#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "motor_driver.h"
#include "PS4Controller.h"

static const char *TAG = "MAIN";

// ESP32のBT MACアドレス（E0:8C:FE:FE:A6:2E）
#define BT_MAC_ADDRESS "E0:8C:FE:FE:A6:2E"
#define STICK_DEADZONE 10

static int16_t scale_stick_to_speed(int8_t stick_val) {
    if (stick_val > -STICK_DEADZONE && stick_val < STICK_DEADZONE) {
        return 0;
    }
    if (stick_val >= STICK_DEADZONE) {
        int scaled = (int)(stick_val - STICK_DEADZONE) * 255 / (127 - STICK_DEADZONE);
        return (scaled > 255) ? 255 : (int16_t)scaled;
    } else {
        int scaled = (int)(stick_val + STICK_DEADZONE) * 255 / (128 - STICK_DEADZONE);
        return (scaled < -255) ? -255 : (int16_t)scaled;
    }
}

static void print_esp32_bt_mac_address(void) {
    uint8_t bt_mac[6] = {0};
    esp_err_t ret = esp_read_mac(bt_mac, ESP_MAC_BT);
    
    ESP_LOGI(TAG, "==================================================");
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, " ESP32 Bluetooth MAC Address (BD_ADDR):");
        ESP_LOGI(TAG, " >>  %02X:%02X:%02X:%02X:%02X:%02X  <<",
                 bt_mac[0], bt_mac[1], bt_mac[2], bt_mac[3], bt_mac[4], bt_mac[5]);
        ESP_LOGI(TAG, " (PS4コントローラのペアリング時にこのMACアドレスを指定してください)");
    } else {
        ESP_LOGE(TAG, " Failed to read Bluetooth MAC Address!");
    }
    ESP_LOGI(TAG, "==================================================");
}

void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32 ARC2026 Robot Control Starting ===");

    // ESP32自身のBluetooth MACアドレスを表示
    print_esp32_bt_mac_address();

    // NVS初期化
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // モータドライバ初期化
    motor_driver_init(NULL);

    // PS4Controller ライブラリの開始
    ESP_LOGI(TAG, "Starting PS4Controller library (Target MAC: %s)...", BT_MAC_ADDRESS);
    ps4_wrapper_begin(BT_MAC_ADDRESS);

    ESP_LOGI(TAG, "Robot control loop ready. Waiting for PS4 Controller...");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms (20Hz) 制御周期

        if (ps4_wrapper_is_connected()) {
            // スティック入力取得 (LY: 上でマイナス・下でプラスのため符号反転して前進正にする)
            int8_t ly = ps4_wrapper_get_lstick_y();
            int8_t rx = ps4_wrapper_get_rstick_x();

            int16_t forward = scale_stick_to_speed(-ly);
            int16_t turn = scale_stick_to_speed(rx);

            int left_speed = forward + turn;
            int right_speed = forward - turn;

            if (left_speed > 255) left_speed = 255;
            if (left_speed < -255) left_speed = -255;
            if (right_speed > 255) right_speed = 255;
            if (right_speed < -255) right_speed = -255;

            motor_set_speed(MOTOR_LEFT, (int16_t)left_speed);
            motor_set_speed(MOTOR_RIGHT, (int16_t)right_speed);
        } else {
            // 切断時は全モータ停止
            motor_stop_all();
        }
    }
}


